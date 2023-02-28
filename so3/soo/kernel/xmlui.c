/*
 * Copyright (C) 2021 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <soo/xmlui.h>
#include <roxml.h>
#include <string.h>

/**
 * Prepare a XML message for sending to the UI app
 *
 * @param buffer	buffer allocated by the caller
 * @param id		unique ID which identifies the message
 * @param value		Message content
 */
void xml_prepare_message(char *buffer, char *id, char *value) {

	char *__buffer;
	node_t *messages, *msg;

	/* Adding the messages node */
	messages = roxml_add_node(NULL, 0, ROXML_ELM_NODE, "messages", NULL);

	/* Adding the message itself */
	msg = roxml_add_node(messages, 0, ROXML_ELM_NODE, "message", NULL);

	roxml_add_node(msg, 0, ROXML_ATTR_NODE, "to", id);

	roxml_add_node(msg, 0, ROXML_TXT_NODE, NULL, value);

	roxml_commit_changes(messages, NULL, &__buffer, 1);

	strcpy(buffer, __buffer);

	roxml_release(RELEASE_LAST);
	roxml_close(messages);

}

/**
 * Retrieve the content of an event message
 *
 * @param buffer	The source event message
 * @param id		The ID of this event
 * @param action	The action of this event message
 */
void xml_parse_event(char *buffer, char *id, char *action) {

	node_t *root, *xml;
	node_t *events, *event, *__from, *__action;

	root = roxml_load_buf(buffer);
	xml =  roxml_get_chld(root, NULL,  0);

	events = roxml_get_chld(root, "events", 0);
	event = roxml_get_chld(events, "event", 0);
	__from = roxml_get_attr(event, "from", 0);
	__action = roxml_get_attr(event, "action", 0);


	strcpy(id, roxml_get_content(__from, NULL, 0, NULL));
	strcpy(action, roxml_get_content(__action, NULL, 0, NULL));

	roxml_release(RELEASE_LAST);
	roxml_close(root);

}

void xml_get_event_content(char *buffer, char *content) {

	node_t *root;
	node_t *events, *event;

	root = roxml_load_buf(buffer);

	events = roxml_get_chld(root, "events", 0);
	event = roxml_get_chld(events, "event", 0);

	strcpy(content, roxml_get_content(event, NULL, 0, NULL));

	roxml_release(RELEASE_LAST);
	roxml_close(root);
}

/**
 * Prepare a chat message for the SOO.chat ME
 * 
 * @param buffer	Already allocated buffer which will receive the XML message with a chat inside
 * @param sender	The sender slotID
 * 
 */ 
void xml_prepare_chat(char *buffer, char *sender, char *text) {

	char chat_xml[512] = { 0 };

	sprintf(chat_xml, "<chat from=\"%s\">%s</chat>", sender, text);
	xml_prepare_message(buffer, SCROLL_ID, chat_xml);
}
