/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "kd_imgsensor.h"

#include "regulator/regulator.h"
#include "gpio/gpio.h"
/*#include "mt6306/mt6306.h"*/
#include "mclk/mclk.h"



#include "imgsensor_cfg_table.h"

enum IMGSENSOR_RETURN
	(*hw_open[IMGSENSOR_HW_ID_MAX_NUM])(struct IMGSENSOR_HW_DEVICE **) = {
	imgsensor_hw_regulator_open,
	imgsensor_hw_gpio_open,
	/*imgsensor_hw_mt6306_open,*/
	imgsensor_hw_mclk_open
};

struct IMGSENSOR_HW_CFG imgsensor_custom_config[] = {
	{
		IMGSENSOR_SENSOR_IDX_MAIN,
		IMGSENSOR_I2C_DEV_0,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AFVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_SUB,
		IMGSENSOR_I2C_DEV_1,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
            {IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_DVDD},
			//{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_MAIN2,
		IMGSENSOR_I2C_DEV_2,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			//{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_SUB2,
		IMGSENSOR_I2C_DEV_1,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_MAIN3,
		IMGSENSOR_I2C_DEV_2,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AFVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},

	{IMGSENSOR_SENSOR_IDX_NONE}
};

struct IMGSENSOR_HW_POWER_SEQ platform_power_sequence[] = {
#ifdef MIPI_SWITCH
	{
		PLATFORM_POWER_SEQ_NAME,
		{
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_EN,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
				0
			},
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL,
				IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0
			},
		},
		IMGSENSOR_SENSOR_IDX_SUB,
	},
	{
		PLATFORM_POWER_SEQ_NAME,
		{
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_EN,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
				0
			},
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0
			},
		},
		IMGSENSOR_SENSOR_IDX_MAIN2,
	},
#endif

	{NULL}
};

/* Legacy design */
struct IMGSENSOR_HW_POWER_SEQ sensor_power_sequence[] = {
/*blackjack*/
#if defined(S5K3P9SXT_MIPI_RAW)
        {
                SENSOR_DRVNAME_S5K3P9SXT_MIPI_RAW,
                {
                        {RST, Vol_Low, 1},
                        {DOVDD, Vol_1800, 0},
                        {AVDD, Vol_2800, 0},
                        {DVDD, Vol_1000, 0},
                        {AFVDD, Vol_2800, 5},
                        //{PDN, Vol_Low, 4},
                        //{PDN, Vol_High, 0},
                        {RST, Vol_High, 0},
                        {SensorMCLK, Vol_High, 0},
                },
        },
#endif
#if defined(S5K3P9SX_MIPI_RAW)
        {
                SENSOR_DRVNAME_S5K3P9SX_MIPI_RAW,
                {
                        {RST, Vol_Low, 1},
                        {DOVDD, Vol_1800, 0},
                        {AVDD, Vol_2800, 0},
                        {DVDD, Vol_1000, 0},
                        {AFVDD, Vol_2800, 5},
                        //{PDN, Vol_Low, 4},
                        //{PDN, Vol_High, 0},
                        {RST, Vol_High, 0},
                        {SensorMCLK, Vol_High, 0},
                },
        },
#endif

#if defined(BLACKJACK_HLT_OV16A10_MIPI_RAW)
		 {SENSOR_DRVNAME_BLACKJACK_HLT_OV16A10_MIPI_RAW,
		  {
			   {SensorMCLK, Vol_High, 0},
			   {RST, Vol_Low, 0},
			   {DOVDD, Vol_1800, 0},
			   {AVDD, Vol_2800, 0},
			   {DVDD, Vol_1200, 0},
			   {AFVDD, Vol_2800, 5},
			   {RST, Vol_High, 0},
		  },
		 },
#endif
#if defined(BLACKJACK_TSP_OV16880_MIPI_RAW)
		 {SENSOR_DRVNAME_BLACKJACK_TSP_OV16880_MIPI_RAW,
		  {
			   {SensorMCLK, Vol_High, 3},
			   {RST, Vol_Low, 3},
			   {DOVDD, Vol_1800, 1},
			   {AVDD, Vol_2800, 1},
			   {DVDD, Vol_1200, 1},
			   {AFVDD, Vol_2800, 5},
			   {RST, Vol_High, 0},
		  },
		 },
#endif
#if defined(GC8034_MIPI_RAW)
                 {SENSOR_DRVNAME_GC8034_MIPI_RAW,
                  {
                   {SensorMCLK, Vol_Low, 0},
                   {RST, Vol_Low, 0},
                   {DOVDD, Vol_1800, 1},
                   {DVDD, Vol_1200, 1},
                   {AVDD, Vol_2800, 1},
                   {SensorMCLK, Vol_High, 1},
                   //{PDN, Vol_High, 1},
                   {RST, Vol_High, 1},
                   },
                  },
#endif
#if defined(BLACKJACK_TXD_HI846_MIPI_RAW)
    {
        SENSOR_DRVNAME_BLACKJACK_TXD_HI846_MIPI_RAW,
        {
            {DOVDD, Vol_1800, 1},
            {AVDD, Vol_2800, 1},
            {DVDD, Vol_1200, 5},
            {SensorMCLK, Vol_High, 0},
//            {AFVDD, Vol_2800, 1},
//            {PDN, Vol_Low, 0},
//            {PDN, Vol_High, 0},
            {RST, Vol_Low, 10},
            {RST, Vol_High, 1},
        },
    },
#endif
#if defined(GC2375_MIPI_RAW)
		{
			 SENSOR_DRVNAME_GC2375_MIPI_RAW,
			 {
				{RST, Vol_Low, 1},
				{PDN, Vol_High, 6,Vol_Low, 0},
				{DOVDD, Vol_1800, 0},
				{AVDD, Vol_2800, 0},
				{SensorMCLK, Vol_High, 0},
				{PDN, Vol_Low,1,Vol_High, 0},
				{RST, Vol_High, 0},
				{PDN, Vol_Low, 1,Vol_High, 1},
			},
		},
#endif
#if defined(BLACKJACK_TSP_GC2375_MIPI_RAW)
		{
			 SENSOR_DRVNAME_BLACKJACK_TSP_GC2375_MIPI_RAW,
			 {
				{RST, Vol_Low, 1},
				{PDN, Vol_High, 6,Vol_Low, 0},
				{DOVDD, Vol_1800, 0},
				{AVDD, Vol_2800, 0},
				{SensorMCLK, Vol_High, 0},
				{PDN, Vol_Low,1,Vol_High, 0},
				{RST, Vol_High, 0},
				{PDN, Vol_Low, 1,Vol_High, 1},
			},
		},
#endif
#if defined(BJ_TSPGC2375_TXD3P9_MIPI_RAW)
		{
			 SENSOR_DRVNAME_BJ_TSPGC2375_TXD3P9_MIPI_RAW,
			 {
				{RST, Vol_Low, 1},
				{PDN, Vol_High, 6,Vol_Low, 0},
				{DOVDD, Vol_1800, 0},
				{AVDD, Vol_2800, 0},
				{SensorMCLK, Vol_High, 0},
				{PDN, Vol_Low,1,Vol_High, 0},
				{RST, Vol_High, 0},
				{PDN, Vol_Low, 1,Vol_High, 1},
			},
		},
#endif
#if defined(BLACKJACK_SUN_GC02M1B_MIPI_RAW)
    {SENSOR_DRVNAME_BLACKJACK_SUN_GC02M1B_MIPI_RAW,
        {
            {RST, Vol_Low, 1},
            {PDN, Vol_High, 6,Vol_Low, 0},
            {DOVDD, Vol_1800, 0},
            {AVDD, Vol_2800, 0},
            {SensorMCLK, Vol_High, 0},
            {PDN, Vol_Low,1,Vol_High, 0},
            {RST, Vol_High, 0},
            {PDN, Vol_Low, 1,Vol_High, 1},
        },
    },
#endif
#if defined(BJ_SUN_GC02M1B_16880_MIPI_RAW)
    {SENSOR_DRVNAME_BJ_SUN_GC02M1B_16880_MIPI_RAW,
        {
            {RST, Vol_Low, 1},
            {PDN, Vol_High, 6,Vol_Low, 0},
            {DOVDD, Vol_1800, 0},
            {AVDD, Vol_2800, 0},
            {SensorMCLK, Vol_High, 0},
            {PDN, Vol_Low,1,Vol_High, 0},
            {RST, Vol_High, 0},
            {PDN, Vol_Low, 1,Vol_High, 1},
        },
    },
#endif
#if defined(BLACKJACK_TSP_GC02M1B_MIPI_RAW)
    {SENSOR_DRVNAME_BLACKJACK_TSP_GC02M1B_MIPI_RAW,
        {
            {RST, Vol_Low, 1},
            {PDN, Vol_High, 6,Vol_Low, 0},
            {DOVDD, Vol_1800, 0},
            {AVDD, Vol_2800, 0},
            {SensorMCLK, Vol_High, 0},
            {PDN, Vol_Low,1,Vol_High, 0},
            {RST, Vol_High, 0},
            {PDN, Vol_Low, 1,Vol_High, 1},
        },
    },
#endif
#if defined(BLACKJACK_SUN_GC02M1C_MIPI_RAW)
    {SENSOR_DRVNAME_BLACKJACK_SUN_GC02M1C_MIPI_RAW,
        {
            {RST, Vol_Low, 1},
            {PDN, Vol_High, 6,Vol_Low, 0},
            {DOVDD, Vol_1800, 0},
            {AVDD, Vol_2800, 0},
            {SensorMCLK, Vol_High, 0},
            {PDN, Vol_Low,1,Vol_High, 0},
            {RST, Vol_High, 0},
            {PDN, Vol_Low, 1,Vol_High, 1},
        },
    },
#endif
#if defined(BLACKJACK_TSP_GC2375H_MIPI_RAW)
		 {SENSOR_DRVNAME_BLACKJACK_TSP_GC2375H_MIPI_RAW,
		  {
		   {RST, Vol_Low, 1},
		   {PDN, Vol_High, 6,Vol_Low, 0},
		   {DOVDD, Vol_1800, 0},
		  // {DVDD, Vol_1800, 0}, dovdd same
		   {AVDD, Vol_2800, 0},
		   {SensorMCLK, Vol_High, 0},
		   {PDN, Vol_Low,1,Vol_High, 0},
		   {RST, Vol_High, 0},
		   {PDN, Vol_Low, 1,Vol_High, 1},
		   },
		  },
#endif
#if defined(BLACKJACK_JSL_GC2375H_MIPI_RAW)
		 {SENSOR_DRVNAME_BLACKJACK_JSL_GC2375H_MIPI_RAW,
		  {
		   {RST, Vol_Low, 1},
		   {PDN, Vol_High, 6,Vol_Low, 0},
		   {DOVDD, Vol_1800, 0},
		  // {DVDD, Vol_1800, 0}, dovdd same
		   {AVDD, Vol_2800, 0},
		   {SensorMCLK, Vol_High, 0},
		   {PDN, Vol_Low,1,Vol_High, 0},
		   {RST, Vol_High, 0},
		   {PDN, Vol_Low, 1,Vol_High, 1},
		   },
		  },
#endif
#if defined(BLACKJACK_SEA_MT9D015_MIPI_RAW)
    {
        SENSOR_DRVNAME_BLACKJACK_SEA_MT9D015_MIPI_RAW,
        {
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
            {AVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 0},
			{RST, Vol_High, 1},
        },
    },
#endif
/*blackjack end*/
/*AK57 start */
#if defined(OV13855_MIPI_RAW)
			 {SENSOR_DRVNAME_OV13855_MIPI_RAW,
			  {
			   {SensorMCLK, Vol_High, 0},
			   {RST, Vol_Low, 0},
			   {DOVDD, Vol_1800, 0},
			   {AVDD, Vol_2800, 0},
			   {DVDD, Vol_1200, 0},
			   {AFVDD, Vol_2800, 2},
			   {RST, Vol_High, 0},
			   },
			  },
#endif
#if defined(GC5035_MIPI_RAW) 
		 {SENSOR_DRVNAME_GC5035_MIPI_RAW,
		  {
		   {SensorMCLK, Vol_High, 0},
		   {RST, Vol_Low, 0},
		   {DOVDD, Vol_1800, 0}, 
		   {DVDD, Vol_1200, 0},
		   {AVDD, Vol_2800, 1},
		   {RST, Vol_High, 0},
		   },
		  },
#endif
#if defined(GC2375H_MIPI_RAW)
		{
			 SENSOR_DRVNAME_GC2375H_MIPI_RAW,
			 {
				{RST, Vol_Low, 1},
				{PDN, Vol_High, 6,Vol_Low, 0},
				{DOVDD, Vol_1800, 0},
				{AVDD, Vol_2800, 0},
				{SensorMCLK, Vol_High, 0},
				{PDN, Vol_Low,1,Vol_High, 0},
				{RST, Vol_High, 0},
				{PDN, Vol_Low, 1,Vol_High, 1},
			},
		},
#endif
#if defined(GC02M1_MIPI_RAW)
    {SENSOR_DRVNAME_GC02M1_MIPI_RAW,
        {
            {PDN,Vol_Low, 0},
            {RST,Vol_Low, 0},
            {DOVDD, Vol_1800, 1},
            {AVDD, Vol_2800, 0},
            {SensorMCLK, Vol_High, 1},
            {PDN,Vol_High, 0},
            {RST, Vol_High, 0},
        },
    },
#endif
/*AK57 end*/
/* melta start*/
#if defined(MELTA_S5KGM1ST_MIPI_RAW)
	{SENSOR_DRVNAME_MELTA_S5KGM1ST_MIPI_RAW,
	 {
	  {RST, Vol_Low, 0},
	  {DVDD, Vol_1200, 0},
	  {AVDD, Vol_2800, 0},
	  {DOVDD, Vol_1800, 0},
	  {AFVDD, Vol_2800, 0},
	  {RST, Vol_High, 1},
	  {SensorMCLK, Vol_High, 0},
	  },
	 },
#endif
#if defined(MALTALITE_TXD_S5K3L6_MIPI_RAW)
	{
		SENSOR_DRVNAME_MALTALITE_TXD_S5K3L6_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1100, 0},
			//{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 1},
			{SensorMCLK, Vol_High, 1},
		},
	},
#endif
#if defined(MALTALITE_WIN_OV13B10_MIPI_RAW)
	{
		SENSOR_DRVNAME_MALTALITE_WIN_OV13B10_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			//{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 1},
		},
	},
#endif
#if defined(MELTA_SEA_GC5035_MIPI_RAW) 
		 {SENSOR_DRVNAME_MELTA_SEA_GC5035_MIPI_RAW,
		  {
		   {SensorMCLK, Vol_High, 0},
		   {RST, Vol_Low, 0},
		   {DOVDD, Vol_1800, 0}, 
		   {DVDD, Vol_1200, 3},
		   {AVDD, Vol_2800, 1},
		   {RST, Vol_High, 0},
		   },
		  },
#endif
#if defined(MELTA_SUN_GC5035_MIPI_RAW) 
		 {SENSOR_DRVNAME_MELTA_SUN_GC5035_MIPI_RAW,
		  {
		   {SensorMCLK, Vol_High, 0},
		   {RST, Vol_Low, 0},
		   {DOVDD, Vol_1800, 0}, 
		   {DVDD, Vol_1200, 3},
		   {AVDD, Vol_2800, 1},
		   {RST, Vol_High, 0},
		   },
		  },
#endif
#if defined(MALTALITE_SEA_GC5035_MIPI_RAW) 
		 {SENSOR_DRVNAME_MALTALITE_SEA_GC5035_MIPI_RAW,
		  {
		   {SensorMCLK, Vol_High, 0},
		   {RST, Vol_Low, 0},
		   {DOVDD, Vol_1800, 0}, 
		   {DVDD, Vol_1200, 3},
		   {AVDD, Vol_2800, 1},
		   {RST, Vol_High, 0},
		   },
		  },
#endif
#if defined(MALTALITE_SUN_SP5506_MIPI_RAW) 
		 {SENSOR_DRVNAME_MALTALITE_SUN_SP5506_MIPI_RAW,
		  {
		   {SensorMCLK, Vol_High, 0},
		   {RST, Vol_Low, 0},
		   {DOVDD, Vol_1800, 0}, 
		   {DVDD, Vol_1200, 3},
		   {AVDD, Vol_2800, 1},
		   {RST, Vol_High, 0},
		   },
		  },
#endif
#if defined(MALTA_SEA_GC02M1_MIPI_RAW)
    {SENSOR_DRVNAME_MALTA_SEA_GC02M1_MIPI_RAW,
        {
            {PDN,Vol_Low, 0},
            {RST,Vol_Low, 0},
            {DOVDD, Vol_1800, 9},
            {AVDD, Vol_2800, 0},
            {SensorMCLK, Vol_High, 1},
            {PDN,Vol_High, 0},
            {RST, Vol_High, 0},
        },
    },
#endif
#if defined(MALTA_SUN_OV02B10_MIPI_RAW)
    {SENSOR_DRVNAME_MALTA_SUN_OV02B10_MIPI_RAW,
        {
            {PDN,Vol_Low, 0},
            {RST,Vol_Low, 0},
            {DOVDD, Vol_1800, 9},
            {AVDD, Vol_2800, 1},
            {PDN,Vol_Low, 0,Vol_Low,0},
            {SensorMCLK, Vol_High, 1},
            {PDN,Vol_High, 5,Vol_High,0},
            {RST, Vol_High, 4},
        },
    },
#endif
/* melta end*/
#if defined(IMX398_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX398_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1},
		},
	},
#endif
#if defined(OV23850_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV23850_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 2},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(IMX386_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX386_MIPI_RAW,
		{
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1100, 0},
			{DOVDD, Vol_1800, 0},
			{AFVDD, Vol_2800, 1},
			{SensorMCLK, Vol_High, 1},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(IMX386_MIPI_MONO)
	{
		SENSOR_DRVNAME_IMX386_MIPI_MONO,
		{
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1100, 0},
			{DOVDD, Vol_1800, 0},
			{AFVDD, Vol_2800, 1},
			{SensorMCLK, Vol_High, 1},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5},
		},
	},
#endif

#if defined(IMX338_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX338_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2500, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(S5K4E6_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K4E6_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2900, 0},
			{DVDD, Vol_1200, 2},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K3P8SP_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3P8SP_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
		},
	},
#endif
#if defined(S5K2T7SP_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2T7SP_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
		},
	},
#endif
#if defined(IMX230_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX230_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_2500, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 1},
			{SensorMCLK, Vol_High, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 10}
		},
	},
#endif
#if defined(S5K3M2_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3M2_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K3P3SX_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3P3SX_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K5E2YA_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K5E2YA_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K4ECGX_MIPI_YUV)
	{
		SENSOR_DRVNAME_S5K4ECGX_MIPI_YUV,
		{
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{DOVDD, Vol_1800, 1},
			{AFVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(OV16880_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV16880_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(S5K2P7_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2P7_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1000, 1},
			{DOVDD, Vol_1800, 1},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_Low, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
		},
	},
#endif
#if defined(S5K2P8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2P8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX258_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX258_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX258_MIPI_MONO)
	{
		SENSOR_DRVNAME_IMX258_MIPI_MONO,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX377_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX377_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(OV8858_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV8858_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(OV8856_MIPI_RAW)
	{SENSOR_DRVNAME_OV8856_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 2},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(S5K2X8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2X8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX214_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX214_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(IMX214_MIPI_MONO)
	{
		SENSOR_DRVNAME_IMX214_MIPI_MONO,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(S5K3L8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3L8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX362_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX362_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K2L7_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2L7_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 3},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(IMX318_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX318_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(OV8865_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV8865_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(IMX219_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX219_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1000, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K3M3_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3M3_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(OV5670_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV5670_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(OV5670_MIPI_RAW_2)
	{
		SENSOR_DRVNAME_OV5670_MIPI_RAW_2,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(OV20880_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV20880_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{RST, Vol_Low, 1},
			{AVDD, Vol_2800, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1100, 1},
			{RST, Vol_High, 5}
		},
	},
#endif
	/* add new sensor before this line */
	{NULL,},
};

