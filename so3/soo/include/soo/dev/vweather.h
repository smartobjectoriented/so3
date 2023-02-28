/*
 * Copyright (C) 2018 Baptiste Delporte <bonel@bonel.net>
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

#ifndef VWEATHER_H
#define VWEATHER_H

#include <types.h>

#include <soo/soo.h>
#include <soo/ring.h>
#include <soo/grant_table.h>
#include <soo/vdevfront.h>

#define VWEATHER_NAME		"vweather"
#define VWEATHER_PREFIX		"[" VWEATHER_NAME "] "

#define VWEATHER_DEV_MAJOR	120
#define VWEATHER_DEV_NAME	"/dev/vweather"

/* ASCII data coming from the weather station */
typedef struct {
	char	frame_begin[1];	/* 'W' */
	char	temperature[5];	/* Outdoor temperature in Celsius degrees: '+'/'-'ab.c */
	char	south_sun[2];	/* South Sun in klx: ab */
	char	west_sun[2];	/* West Sun in klx: ab */
	char	east_sun[2];	/* East Sun in klx: ab */
	char	twilight[1];	/* Twilight: 'J'/'N' */
	char	light[3];	/* Light intensity in lx: abc */
	char	wind[4];	/* Wind speed in m/s: ab.c */
	char	rain[1];	/* Rain: 'J'/'N' */
	char	week_day[1];	/* Day of the week: '1'..'7', from Monday to Sunday */
	char	day[2];		/* Day */
	char	month[2];	/* Month */
	char	year[2];	/* Year on two digits */
	char	hour[2];	/* Hour */
	char	minute[2];	/* Minute */
	char	second[2];	/* Second */
	char	summer_time[1];	/* Summer time: 'J'/'N'/'?' */
	char	checksum[4];	/* Checksum: abcd */
	char	frame_end[1];	/* 3 */
} vweather_ascii_data_t;

/* Rain intensity */
typedef enum {
	NO_RAIN,	/* 0mm/hr */
	LIGHT_RAIN,	/* <3mm/hr */
	MODERATE_RAIN,	/* >=3 <8mm/hr */
	HEAVY_RAIN	/* >=8mm/h */
} rain_intensity_t ;

/* Weather data spread over the ecosystem */
typedef struct {
	uint32_t		south_sun;	/* South Sun in klx */
	uint32_t		west_sun;	/* West Sun in klx */
	uint32_t		east_sun;	/* East Sun in klx */
	uint32_t		light;		/* Light in lx */
	int32_t			temperature;	/* Temperature in Celsius degrees */
	uint32_t		wind;		/* Wind speed in m/s */
	rain_intensity_t	rain_intensity;	/* Rain intensity */
	uint8_t			twilight;	/* Twilight: 1 or 0 */
	uint8_t			rain;		/* Rain: 1 or 0 */
} vweather_data_t;

/* 40 bytes */
#define VWEATHER_ASCII_DATA_SIZE	sizeof(vweather_ascii_data_t)

/* Data accessor */
#define VWEATHER_GET_ASCII_DATA(output, data_set, name) \
	memcpy(output, &data_set.name[0], sizeof(data_set.name)); \
	output[sizeof(data_set.name)] = '\0';

#define VWEATHER_DATA_SIZE	sizeof(vweather_data_t)

typedef void (*vweather_interrupt_t)(void);

void vweather_register_interrupt(vweather_interrupt_t update_interrupt);

vweather_data_t *vweather_get_data(void);


typedef struct {

	vdevfront_t vdevfront;

	bool		connected;
	struct vbus_device  *dev;

	uint32_t	evtchn;
	uint32_t	irq;

	char		*weather_data;
	unsigned int	weather_pfn;

} vweather_t;

static inline vweather_t *to_vweather(struct vbus_device *vdev) {
	vdevfront_t *vdevback = dev_get_drvdata(vdev->dev);
	return container_of(vdevback, vweather_t, vdevfront);
}


/* ISR associated to the notification */
irq_return_t vweather_update_interrupt(int irq, void *dev_id);

/*
 * Interface with the client.
 * This function must be provided in the applicative part.
 */
void weather_data_update_interrupt(void);


/* Processing and connected state management */
void vweather_start(void);
void vweather_end(void);
bool vweather_is_connected(void);


#endif /* VWEATHER_H */
