/*
 * Copyright (C) 2022 David Truan <david.truan@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#if 0
#define DEBUG
#endif

/* Activate to add a first test chat at the ME boot */ 
#define TEST_CHAT 0

#include <mutex.h>
#include <delay.h>
#include <timer.h>
#include <heap.h>
#include <memory.h>

#include <soo/avz.h>
#include <soo/gnttab.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/debug.h>
#include <soo/debug/dbgvar.h>
#include <soo/debug/logbool.h>
#include <soo/evtchn.h>
#include <soo/xmlui.h>
#include <soo/dev/vuihandler.h>

#include <me/chat.h>

/* Contains the current chat message. */
char cur_text[MAX_MSG_LENGTH];

/**
 * Contains the history from the last messages each SO has sent.
 * Used to compare arriving messages and keep track of who sent us what
 */
static LIST_HEAD(chat_history);


/********* History Management ********************/

/**
 * @brief Check if the sender, defined by senderUID, is in the history
 * 
 * @param senderUID UID of the sender SOO we are checking the presence
 * @return bool true if the sender is present, false otherwise
 */ 
bool sender_is_in_history(uint64_t senderUID) {
	chat_t *chat;

	list_for_each_entry(chat, &chat_history, list) {
		if (chat->chat_entry.originUID == senderUID) {
			return true;
		}
	}		
	return false;
}

/**
 * @brief Get the chat specified by the senderUID
 * 
 * @param senderUID UID of the sender which sent the chat
 * @return chat_entry_t* The last chat by the sender, NULL if it does not exist
 */
chat_entry_t *find_chat_from_sender_in_history(uint64_t senderUID) {
	chat_t *chat;

	list_for_each_entry(chat, &chat_history, list)
		if (chat->chat_entry.originUID == senderUID)
			return &chat->chat_entry;

	return NULL;
}


/**
 * @brief Update an existing chat in the history. 
 * IT SHOULD NOT BE USED DIRECTLY, as it is used by add_chat_in_history.
 * 
 * @param updated_chat A chat_entry_t struct containing the chat to update
 * @return bool true if the chat was updated, false otherwise
 */
void update_chat_in_history(chat_entry_t *updated_chat) {
	chat_entry_t *chat_entry = find_chat_from_sender_in_history(updated_chat->originUID);

	BUG_ON(!chat_entry);

	chat_entry->stamp = updated_chat->stamp;
	strcpy(chat_entry->text, updated_chat->text);
}

/**
 * @brief Add a chat to the history. 
 * It is the only function you need to add a chat, as it checks if it needs to
 * add it or to call update_chat_in_history to update it if a chat from the sender
 * is already present
 * 
 * @param new_chat A chat_entry_t struct containing the chat to add
 */
void add_chat_in_history(chat_entry_t *new_chat) {
	chat_t *chat;

	/* First, we check if the history already contains a message from 
	this sender. If so, we just update the chat */
	if (sender_is_in_history(new_chat->originUID)) {
		update_chat_in_history(new_chat);
		return;
	}

	/* If no chat from this sender is present, add the chat */
	chat = malloc(sizeof(chat_t));
	BUG_ON(!chat);

	chat->chat_entry.originUID = new_chat->originUID;
	chat->chat_entry.stamp = new_chat->stamp;

	strcpy(chat->chat_entry.text, new_chat->text);

	list_add_tail(&chat->list, &chat_history);
}


/**
 * Check if the chat is already present. 
 * 
 * @param chat chat_entry_t* The chat we want to check the presence
 * @return bool true if the exact same chat is present (originUID, stamp and text are the same)
 * 
 */ 
bool is_chat_in_history(chat_entry_t *chat) {
	chat_entry_t *chat_entry = find_chat_from_sender_in_history(chat->originUID);

	if (chat_entry == NULL) {
		printk("No chat from this sender was found.\n");
		return false;
	}

	/* If the chat from this sender is the same (same stamp and same text) 
	it means that the chat is already present in the history */
	if (chat_entry->stamp == chat->stamp && !strcmp(chat_entry->text, chat->text)) {
		return true;
	}

	return false;
}

/** 
 * @brief send a chat to the connected tablet
 * 
 * @param senderUID: UID of the sender (the SO connected to the tablet)
 * @param text: The chat text itself
 * 
 */ 
void send_chat_to_tablet(char *sender_name, char *text) {
	char msg[MAX_MSG_LENGTH];

	xml_prepare_chat(msg, sender_name, text);
	vuihandler_send(msg, strlen(msg)+1, VUIHANDLER_POST);
}

/**
 * @brief Sends the model to the tablet 
 * 
 */ 
void send_chat_model(void) {
	vuihandler_send(CHAT_MODEL, strlen(CHAT_MODEL)+1, VUIHANDLER_SELECT);
}

/**
 *
 * @param args - To be compliant... Actually not used.
 * @return
 */
void process_events(char *data, size_t size) {
	char id[ID_MAX_LENGTH];
	char action[ACTION_MAX_LENGTH];
	char content[MAX_MSG_LENGTH];
	char msg[MAX_MSG_LENGTH];

	memset(id, 0, ID_MAX_LENGTH);
	memset(action, 0, ACTION_MAX_LENGTH);
	memset(msg, 0, MAX_MSG_LENGTH);
	memset(content, 0, MAX_MSG_LENGTH);

	xml_parse_event(data, id, action);

	/* If it is a text-edit event, it means the user typed something
	so we save it in the temporary buffer */
	if (!strcmp(id, TEXTEDIT_ID)) {

		xml_get_event_content(data, content);
		strcpy(cur_text, content);
	} else if (!strcmp(id, BTN_SEND_ID) && !strcmp(action, "clickDown")) {
			
		/* We don't send empty text */	
		if (!strcmp(cur_text, "")) return;
		/* Pepare an send the chat message */
		send_chat_to_tablet(sh_chat->soo_name, cur_text);
		
		/* Notify the text-edit widget that it must clear its text */
		memset(msg, 0, MAX_MSG_LENGTH);	
		xml_prepare_message(msg, TEXTEDIT_ID, "");
		vuihandler_send(msg, strlen(msg)+1, VUIHANDLER_POST);


		/* We update the chat_entry_t which will be delivered across the network */
		sh_chat->cur_chat.stamp++;
		strcpy(sh_chat->cur_chat.text, cur_text);
		sh_chat->need_propagate = true;
	}
}

/*
 * The main application of the ME is executed right after the bootstrap. It may be empty since activities can be triggered
 * by external events based on frontend activities.
 */
void *app_thread_main(void *args) {

	/* The ME can cooperate with the others. */
	spad_enable_cooperate();

	printk("Enjoy the SOO.chat ME !\n");

	/* register our process_event callback to the vuihandler */ 
	vuihandler_register_callbacks(send_chat_model, process_events);

	sh_chat->cur_chat.stamp = 0;
	memset(sh_chat->cur_chat.text, 0, MAX_MSG_LENGTH);


#if TEST_CHAT
	sh_chat->cur_chat.stamp = 1;
	sprintf(sh_chat->cur_chat.text, "Test MSG");
#endif

	return NULL;
}
