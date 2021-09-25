/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros*/
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/power_supply.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/scatterlist.h>
#include <linux/suspend.h>
#include <linux/version.h>
#include <linux/i2c.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#endif
#include "upmu_common.h"
#include "bq24157.h"
#include "mtk_charger_intf.h"

static const unsigned int VBAT_CVTH[] = {
	3500000, 3520000, 3540000, 3560000,
	3580000, 3600000, 3620000, 3640000,
	3660000, 3680000, 3700000, 3720000,
	3740000, 3760000, 3780000, 3800000,
	3820000, 3840000, 3860000, 3880000,
	3900000, 3920000, 3940000, 3960000,
	3980000, 4000000, 4020000, 4040000,
	4060000, 4080000, 4100000, 4120000,
	4140000, 4160000, 4180000, 4200000,
	4220000, 4240000, 4260000, 4280000,
	4300000, 4320000, 4340000, 4360000,
	4380000, 4400000, 4420000, 4440000
};

//static const unsigned int CSTH[] = {
//	550000, 650000, 750000, 850000,
//	950000, 1050000, 1150000, 1250000,
//};

static const unsigned int CSTH[] = {
	500000, 550000, 650000, 750000,
	850000, 950000, 1050000, 1250000,
};

/*bq24157 REG00 IINLIM[5:0]*/
static const unsigned int INPUT_CSTH[] = {
	100000, 500000, 800000, 5000000
};

/* bq24157 REG0A BOOST_LIM[2:0], mA */
static const unsigned int BOOST_CURRENT_LIMIT[] = {
	500, 750, 1200, 1400, 1650, 1875, 2150,
};

static u32 max_charge_current=0;

#include <ontim/ontim_dev_dgb.h>
static  char charge_ic_vendor_name[50]="BQ24157";
DEV_ATTR_DECLARE(charge_ic)
DEV_ATTR_DEFINE("vendor",charge_ic_vendor_name)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(charge_ic,charge_ic,8);

#ifdef CONFIG_OF
#else
#define bq24157_SLAVE_ADDR_WRITE0xD4
#define bq24157_SLAVE_ADDR_Read	0xD5
#ifdef I2C_SWITHING_CHARGER_CHANNEL
#define bq24157_BUSNUM I2C_SWITHING_CHARGER_CHANNEL
#else
#define bq24157_BUSNUM 0
#endif
#endif

struct bq24157_info {
	struct charger_device *chg_dev;
	struct power_supply *psy;
	struct charger_properties chg_props;
	struct device *dev;
	struct gtimer otg_kthread_gtimer;
	struct workqueue_struct *otg_boost_workq;
	struct work_struct kick_work;
	unsigned int polling_interval;
	bool polling_enabled;

	const char *chg_dev_name;
	const char *eint_name;
	enum charger_type chg_type;
	int irq;
};

static struct bq24157_info *g_info;
static struct i2c_client *new_client;
static const struct i2c_device_id bq24157_i2c_id[] = { {"bq24157", 0}, {} };

static void enable_boost_polling(bool poll_en);
static void usbotg_boost_kick_work(struct work_struct *work);
static int usbotg_gtimer_func(struct gtimer *data);

static unsigned int charging_value_to_parameter(const unsigned int *parameter, const unsigned int array_size,
					const unsigned int val)
{
	if (val < array_size)
		return parameter[val];
	pr_info("Can't find the parameter\n");
	return parameter[0];
}

static unsigned int charging_parameter_to_value(const unsigned int *parameter, const unsigned int array_size,
					const unsigned int val)
{
	unsigned int i;

	pr_debug_ratelimited("array_size = %d\n", array_size);

	for (i = 0; i < array_size; i++) {
		if (val == *(parameter + i))
			return i;
	}

	pr_info("NO register value match\n");
	/* TODO: ASSERT(0);	// not find the value */
	return 0;
}

static unsigned int bmt_find_closest_level(const unsigned int *pList, unsigned int number,
					 unsigned int level)
{
	unsigned int i;
	unsigned int max_value_in_last_element;

	if (pList[0] < pList[1])
		max_value_in_last_element = 1;
	else
		max_value_in_last_element = 0;

	if (max_value_in_last_element == 1) {
		for (i = (number - 1); i != 0; i--) {	/* max value in the last element */
			if (pList[i] <= level) {
				pr_debug_ratelimited("zzf_%d<=%d, i=%d\n", pList[i], level, i);
				return pList[i];
			}
		}
		pr_info("Can't find closest level\n");
		return pList[0];
		/* return 000; */
	} else {
		for (i = 0; i < number; i++) {	/* max value in the first element */
			if (pList[i] <= level)
				return pList[i];
		}
		pr_info("Can't find closest level\n");
		return pList[number - 1];
		/* return 000; */
	}
}

unsigned char bq24157_reg[BQ24157_REG_NUM] = { 0 };
static DEFINE_MUTEX(bq24157_i2c_access);
static DEFINE_MUTEX(bq24157_access_lock);
#define BQ24157_SLAVE_ADDR 0x6a
static int bq24157_read_byte(u8 reg_addr, u8 *rd_buf, int rd_len)
{
	int ret = 0;
	struct i2c_adapter *adap = new_client->adapter;
	struct i2c_msg msg[2];
	u8 *w_buf = NULL;
	u8 *r_buf = NULL;

	memset(msg, 0, 2 * sizeof(struct i2c_msg));

	w_buf = kzalloc(1, GFP_KERNEL);
	if (w_buf == NULL)
		return -1;
	r_buf = kzalloc(rd_len, GFP_KERNEL);
	if (r_buf == NULL)
		return -1;

	*w_buf = reg_addr;

	msg[0].addr = BQ24157_SLAVE_ADDR;//new_client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = w_buf;

	msg[1].addr = BQ24157_SLAVE_ADDR;//new_client->addr;
	msg[1].flags = 1;
	msg[1].len = rd_len;
	msg[1].buf = r_buf;

	ret = i2c_transfer(adap, msg, 2);

	memcpy(rd_buf, r_buf, rd_len);

	kfree(w_buf);
	kfree(r_buf);
	return ret;
}

int bq24157_write_byte(unsigned char reg_num, u8 *wr_buf, int wr_len)
{
	int ret = 0;
	struct i2c_adapter *adap = new_client->adapter;
	struct i2c_msg msg;
	u8 *w_buf = NULL;

	memset(&msg, 0, sizeof(struct i2c_msg));

	w_buf = kzalloc(wr_len, GFP_KERNEL);
	if (w_buf == NULL)
		return -1;

	w_buf[0] = reg_num;
	memcpy(w_buf + 1, wr_buf, wr_len);

	msg.addr = BQ24157_SLAVE_ADDR;//new_client->addr;
	msg.flags = 0;
	msg.len = wr_len;
	msg.buf = w_buf;

	ret = i2c_transfer(adap, &msg, 1);

	kfree(w_buf);
	return ret;
}

unsigned int bq24157_read_interface(unsigned char reg_num, unsigned char *val, unsigned char MASK,
				unsigned char SHIFT)
{
	unsigned char bq24157_reg = 0;
	unsigned int ret = 0;

	ret = bq24157_read_byte(reg_num, &bq24157_reg, 1);
	pr_debug_ratelimited("[bq24157_read_interface] Reg[%x]=0x%x\n", reg_num, bq24157_reg);
	bq24157_reg &= (MASK << SHIFT);
	*val = (bq24157_reg >> SHIFT);
	pr_debug_ratelimited("[bq24157_read_interface] val=0x%x\n", *val);

	return ret;
}

unsigned int bq24157_config_interface(unsigned char reg_num, unsigned char val, unsigned char MASK,
					unsigned char SHIFT)
{
	unsigned char bq24157_reg = 0;
	unsigned char bq24157_reg_ori = 0;
	unsigned int ret = 0;

	mutex_lock(&bq24157_access_lock);
	ret = bq24157_read_byte(reg_num, &bq24157_reg, 1);
	bq24157_reg_ori = bq24157_reg;
	bq24157_reg &= ~(MASK << SHIFT);
	bq24157_reg |= (val << SHIFT);
	if (reg_num == BQ24157_CON4)
		bq24157_reg &= ~(1 << CON4_RESET_SHIFT);

	ret = bq24157_write_byte(reg_num, &bq24157_reg, 2);
	mutex_unlock(&bq24157_access_lock);
	pr_debug_ratelimited("[bq24157_config_interface] write Reg[%x]=0x%x from 0x%x\n", reg_num,
			bq24157_reg, bq24157_reg_ori);
	/* Check */
	/* bq24157_read_byte(reg_num, &bq24157_reg, 1); */
	/* printk("[bq24157_config_interface] Check Reg[%x]=0x%x\n", reg_num, bq24157_reg); */

	return ret;
}

/* write one register directly */
unsigned int bq24157_reg_config_interface(unsigned char reg_num, unsigned char val)
{
	unsigned char bq24157_reg = val;

	return bq24157_write_byte(reg_num, &bq24157_reg, 2);
}

void bq24157_set_tmr_rst(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON0),
				(unsigned char)(val),
				(unsigned char)(CON0_TMR_RST_MASK),
				(unsigned char)(CON0_TMR_RST_SHIFT)
				);
}

unsigned int bq24157_get_otg_status(void)
{
	unsigned char val = 0;

	bq24157_read_interface((unsigned char)(BQ24157_CON0),
				(unsigned char *)(&val),
				(unsigned char)(CON0_OTG_MASK),
				(unsigned char)(CON0_OTG_SHIFT)
				);
	return val;
}

void bq24157_set_en_stat(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON0),
				(unsigned char)(val),
				(unsigned char)(CON0_EN_STAT_MASK),
				(unsigned char)(CON0_EN_STAT_SHIFT)
				);
}

unsigned int bq24157_get_chip_status(void)
{
	unsigned char val = 0;

	bq24157_read_interface((unsigned char)(BQ24157_CON0),
				(unsigned char *)(&val),
				(unsigned char)(CON0_STAT_MASK),
				(unsigned char)(CON0_STAT_SHIFT)
				);
	return val;
}

unsigned int bq24157_get_boost_status(void)
{
	unsigned char val = 0;

	bq24157_read_interface((unsigned char)(BQ24157_CON0),
				(unsigned char *)(&val),
				(unsigned char)(CON0_BOOST_MASK),
				(unsigned char)(CON0_BOOST_SHIFT)
				);
	return val;

}

unsigned int bq24157_get_fault_status(void)
{
	unsigned char val = 0;

	bq24157_read_interface((unsigned char)(BQ24157_CON0),
				(unsigned char *)(&val),
				(unsigned char)(CON0_FAULT_MASK),
				(unsigned char)(CON0_FAULT_SHIFT)
				);
	return val;
}

void bq24157_set_input_charging_current(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON1),
				(unsigned char)(val),
				(unsigned char)(CON1_LIN_LIMIT_MASK),
				(unsigned char)(CON1_LIN_LIMIT_SHIFT)
				);
}

unsigned int bq24157_get_input_charging_current(void)
{
	unsigned char val = 0;

	bq24157_read_interface((unsigned char)(BQ24157_CON1),
				(unsigned char *)(&val),
				(unsigned char)(CON1_LIN_LIMIT_MASK),
				(unsigned char)(CON1_LIN_LIMIT_SHIFT)
				);

	return val;
}

void bq24157_set_v_low(unsigned int val)
{

	bq24157_config_interface((unsigned char)(BQ24157_CON1),
				(unsigned char)(val),
				(unsigned char)(CON1_LOW_V_MASK),
				(unsigned char)(CON1_LOW_V_SHIFT)
				);
}

void bq24157_set_te(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON1),
				(unsigned char)(val),
				(unsigned char)(CON1_TE_MASK),
				(unsigned char)(CON1_TE_SHIFT)
				);
}

void bq24157_set_ce(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON1),
				(unsigned char)(val),
				(unsigned char)(CON1_CE_MASK),
				(unsigned char)(CON1_CE_SHIFT)
				);
}

void bq24157_set_hz_mode(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON1),
				(unsigned char)(val),
				(unsigned char)(CON1_HZ_MODE_MASK),
				(unsigned char)(CON1_HZ_MODE_SHIFT)
				);
}

void bq24157_set_opa_mode(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON1),
				(unsigned char)(val),
				(unsigned char)(CON1_OPA_MODE_MASK),
				(unsigned char)(CON1_OPA_MODE_SHIFT)
				);
}

void bq24157_set_oreg(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON2),
				(unsigned char)(val),
				(unsigned char)(CON2_OREG_MASK),
				(unsigned char)(CON2_OREG_SHIFT)
				);
}
void bq24157_set_otg_pl(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON2),
				(unsigned char)(val),
				(unsigned char)(CON2_OTG_PL_MASK),
				(unsigned char)(CON2_OTG_PL_SHIFT)
				);
}
void bq24157_set_otg_en(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON2),
				(unsigned char)(val),
				(unsigned char)(CON2_OTG_EN_MASK),
				(unsigned char)(CON2_OTG_EN_SHIFT)
				);
}

unsigned int bq24157_get_vender_code(void)
{
	unsigned char val = 0;

	bq24157_read_interface((unsigned char)(BQ24157_CON3),
				(unsigned char *)(&val),
				(unsigned char)(CON3_VENDER_CODE_MASK),
				(unsigned char)(CON3_VENDER_CODE_SHIFT)
				);
	return val;
}
unsigned int bq24157_get_pn(void)
{
	unsigned char val = 0;

	bq24157_read_interface((unsigned char)(BQ24157_CON3),
				(unsigned char *)(&val),
				(unsigned char)(CON3_PIN_MASK),
				(unsigned char)(CON3_PIN_SHIFT)
				);
	return val;
}

unsigned int bq24157_get_revision(void)
{
	unsigned char val = 0;

	bq24157_read_interface((unsigned char)(BQ24157_CON3),
				(unsigned char *)(&val),
				(unsigned char)(CON3_REVISION_MASK),
				(unsigned char)(CON3_REVISION_SHIFT)
				);
	return val;
}

void bq24157_set_reset(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON4),
				(unsigned char)(val),
				(unsigned char)(CON4_RESET_MASK),
				(unsigned char)(CON4_RESET_SHIFT)
				);
}

void bq24157_set_iocharge(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON4),
				(unsigned char)(val),
				(unsigned char)(CON4_I_CHR_MASK),
				(unsigned char)(CON4_I_CHR_SHIFT)
				);
}

void bq24157_set_iterm(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON4),
				(unsigned char)(val),
				(unsigned char)(CON4_I_TERM_MASK),
				(unsigned char)(CON4_I_TERM_SHIFT)
				);
}

void bq24157_set_dis_vreg(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON5),
				(unsigned char)(val),
				(unsigned char)(CON5_DIS_VREG_MASK),
				(unsigned char)(CON5_DIS_VREG_SHIFT)
				);
}

void bq24157_set_io_level(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON5),
				(unsigned char)(val),
				(unsigned char)(CON5_IO_LEVEL_MASK),
				(unsigned char)(CON5_IO_LEVEL_SHIFT)
				);
}

unsigned int bq24157_get_sp_status(void)
{
	unsigned char val = 0;

	bq24157_read_interface((unsigned char)(BQ24157_CON5),
				(unsigned char *)(&val),
				(unsigned char)(CON5_SP_STATUS_MASK),
				(unsigned char)(CON5_SP_STATUS_SHIFT)
				);
	return val;
}

unsigned int bq24157_get_en_level(void)
{
	unsigned char val = 0;

	bq24157_read_interface((unsigned char)(BQ24157_CON5),
				(unsigned char *)(&val),
				(unsigned char)(CON5_EN_LEVEL_MASK),
				(unsigned char)(CON5_EN_LEVEL_SHIFT)
				);
	return val;
}

void bq24157_set_vsp(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON5),
				(unsigned char)(val),
				(unsigned char)(CON5_VSP_MASK),
				(unsigned char)(CON5_VSP_SHIFT)
				);
}

void bq24157_set_i_safe(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON6),
				(unsigned char)(val),
				(unsigned char)(CON6_ISAFE_MASK),
				(unsigned char)(CON6_ISAFE_SHIFT)
				);
}

void bq24157_set_v_safe(unsigned int val)
{
	bq24157_config_interface((unsigned char)(BQ24157_CON6),
				(unsigned char)(val),
				(unsigned char)(CON6_VSAFE_MASK),
				(unsigned char)(CON6_VSAFE_SHIFT)
				);
}

static int bq24157_dump_register(struct charger_device *chg_dev)
{
	int i;

	for (i = 0; i < BQ24157_REG_NUM; i++) {
		bq24157_read_byte(i, &bq24157_reg[i], 1);
	}
	pr_err("bq24157 [0x0]=0x%x [0x1]=0x%x [0x2]=0x%x  [0x3]=0x%x [0x4]=0x%x [0x5]=0x%x [0x6]=0x%x \n",
		                      bq24157_reg[0],bq24157_reg[1],bq24157_reg[2],
		                      bq24157_reg[3],bq24157_reg[4],bq24157_reg[5],bq24157_reg[6]);

	return 0;
}

static int bq24157_parse_dt(struct bq24157_info *info, struct device *dev)
{
	struct device_node *np = dev->of_node;

	pr_info("%s\n", __func__);

	if (!np) {
		pr_err("%s: no of node\n", __func__);
		return -ENODEV;
	}

	if (of_property_read_string(np, "charger_name", &info->chg_dev_name) < 0) {
		info->chg_dev_name = "primary_chg";
		pr_warn("%s: no charger name\n", __func__);
	}

	if (of_property_read_string(np, "alias_name", &(info->chg_props.alias_name)) < 0) {
		info->chg_props.alias_name = "bq24157";
		pr_warn("%s: no alias name\n", __func__);
	}
	if (!of_property_read_u32(np, "ichg", &max_charge_current)) {
		pr_warn("%s: max_charge_current=%d;\n", __func__,max_charge_current);
	}


	return 0;
}

static int bq24157_do_event(struct charger_device *chg_dev, unsigned int event, unsigned int args)
{
	if (chg_dev == NULL)
		return -EINVAL;

	pr_info("%s: event = %d\n", __func__, event);

	switch (event) {
	case EVENT_EOC:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_EOC);
		break;
	case EVENT_RECHARGE:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_RECHG);
		break;
	default:
		break;
	}

	return 0;
}

static int bq24157_set_cv_voltage(struct charger_device *chg_dev, u32 cv)
{
	int status = 0;
	unsigned short int array_size;
	unsigned int set_cv_voltage;
	unsigned short int register_value;

	bq24157_config_interface((unsigned char)(BQ24157_CON5),
				(unsigned char)(1),
				(unsigned char)(CON5_FLAG_MASK),
				(unsigned char)(CON5_FLAG_SHIFT)
				);

	/*static kal_int16 pre_register_value; */
	array_size = ARRAY_SIZE(VBAT_CVTH);
	/*pre_register_value = -1; */
	set_cv_voltage = bmt_find_closest_level(VBAT_CVTH, array_size, cv);

	register_value =
	charging_parameter_to_value(VBAT_CVTH, array_size, set_cv_voltage);
	pr_info("charging_set_cv_voltage register_value=0x%x %d %d\n",
	 register_value, cv, set_cv_voltage);
	bq24157_set_oreg(register_value);

	return status;
}

static int bq24157_enable_charging(struct charger_device *chg_dev, bool en)
{
	unsigned int status = 0;

	if (en) {
		bq24157_set_ce(0);
		bq24157_set_hz_mode(0);
		bq24157_set_opa_mode(0);
		bq24157_set_te(1);

		bq24157_set_iterm(2);
		bq24157_set_vsp(3);
		
	} else {
//		bq24157_set_ce(1);
//		bq24157_set_hz_mode(1);
		bq24157_set_cv_voltage(chg_dev,3500000);
}

	return status;
}

static int bq24157_get_current(struct charger_device *chg_dev, u32 *ichg)
{
	int status = 0;
	unsigned int array_size;
	unsigned char reg_value;

	array_size = ARRAY_SIZE(CSTH);
	bq24157_read_interface(0x1, &reg_value, 0x3, 0x6);	/* IINLIM */
	*ichg = charging_value_to_parameter(CSTH, array_size, reg_value);

	return status;
}

static int bq24157_set_current(struct charger_device *chg_dev, u32 current_value)
{
	unsigned int status = 0;
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned int register_value;

	if (current_value <= 500000) {
		bq24157_set_io_level(0);
		register_value = 3;
		bq24157_set_iocharge(register_value);
//		array_size = ARRAY_SIZE(CSTH);
//		set_chr_current = bmt_find_closest_level(CSTH, array_size, current_value);
//		register_value = charging_parameter_to_value(CSTH, array_size, set_chr_current);
//		bq24157_set_iocharge(register_value);
	} else {

		if(max_charge_current != 0  && current_value > max_charge_current)	
		{
			pr_info("%s;%d;%d;\n",__func__,current_value,max_charge_current);
			current_value = max_charge_current;			
		}
		bq24157_set_io_level(0);
		array_size = ARRAY_SIZE(CSTH);
		set_chr_current = bmt_find_closest_level(CSTH, array_size, current_value);
		register_value = charging_parameter_to_value(CSTH, array_size, set_chr_current);
		bq24157_set_iocharge(register_value);
	}

	return status;
}

static int bq24157_get_input_current(struct charger_device *chg_dev, u32 *aicr)
{
	unsigned int status = 0;
	unsigned int array_size;
	unsigned int register_value;

	array_size = ARRAY_SIZE(INPUT_CSTH);
	register_value = bq24157_get_input_charging_current();
	*aicr = charging_parameter_to_value(INPUT_CSTH, array_size, register_value);

	return status;
}

static int bq24157_set_input_current(struct charger_device *chg_dev, u32 current_value)
{
	unsigned int status = 0;
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned int register_value;

	if (current_value > 500000) {
		register_value = 0x3;
	} else {
		array_size = ARRAY_SIZE(INPUT_CSTH);
		set_chr_current = bmt_find_closest_level(INPUT_CSTH, array_size, current_value);
		register_value =
	 charging_parameter_to_value(INPUT_CSTH, array_size, set_chr_current);
	}

	bq24157_set_input_charging_current(register_value);

	return status;
}

static int bq24157_get_charging_status(struct charger_device *chg_dev, bool *is_done)
{
	unsigned int status = 0;
	unsigned int ret_val;

	ret_val = bq24157_get_chip_status();

	if (ret_val == 0x2)
		*is_done = true;
	else
		*is_done = false;

	return status;
}

static int bq24157_reset_watch_dog_timer(struct charger_device *chg_dev)
{
	bq24157_set_tmr_rst(1);
	return 0;
}

static int bq24157_charger_enable_otg(struct charger_device *chg_dev, bool en)
{
	bq24157_set_opa_mode(en);

	bq24157_set_otg_en(en);

	enable_boost_polling(en);
	return 0;
}

static void enable_boost_polling(bool poll_en)
{
	if (g_info) {
		if (poll_en) {
			gtimer_start(&g_info->otg_kthread_gtimer,
				     g_info->polling_interval);
			g_info->polling_enabled = true;
		} else {
			g_info->polling_enabled = false;
			gtimer_stop(&g_info->otg_kthread_gtimer);
		}
	}
}

static void usbotg_boost_kick_work(struct work_struct *work)
{

	struct bq24157_info *boost_manager =
		container_of(work, struct bq24157_info, kick_work);

	pr_debug_ratelimited("usbotg_boost_kick_work\n");

	bq24157_set_tmr_rst(1);

	if (boost_manager->polling_enabled == true)
		gtimer_start(&boost_manager->otg_kthread_gtimer,
			     boost_manager->polling_interval);
}

static int usbotg_gtimer_func(struct gtimer *data)
{
	struct bq24157_info *boost_manager =
		container_of(data, struct bq24157_info,
			     otg_kthread_gtimer);

	queue_work(boost_manager->otg_boost_workq,
		   &boost_manager->kick_work);

	return 0;
}

static struct charger_ops bq24157_chg_ops = {

	/* Normal charging */
	.dump_registers = bq24157_dump_register,
	.enable = bq24157_enable_charging,
	.get_charging_current = bq24157_get_current,
	.set_charging_current = bq24157_set_current,
	.get_input_current = bq24157_get_input_current,
	.set_input_current = bq24157_set_input_current,
	/*.get_constant_voltage = bq24157_get_battery_voreg,*/
	.set_constant_voltage = bq24157_set_cv_voltage,
	.kick_wdt = bq24157_reset_watch_dog_timer,
	.is_charging_done = bq24157_get_charging_status,
	/* OTG */
	.enable_otg = bq24157_charger_enable_otg,
	.event = bq24157_do_event,
};

static ssize_t bq24157_show_registers(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	u8 addr;
	u8 val;
	u8 tmpbuf[200];
	int len;
	int idx = 0;
	int ret;

	idx = snprintf(buf, PAGE_SIZE, "%s:\n", "bq24157 Reg");
	for (addr = 0x0; addr <= 0x06; addr++) {
		ret = bq24157_read_byte(addr, &val,1);
		if (ret == 0) {
			len = snprintf(tmpbuf, PAGE_SIZE - idx,
				       "Reg[%.2x] = 0x%.2x\n", addr, val);
			memcpy(&buf[idx], tmpbuf, len);
			idx += len;
		}
	}

	return idx;
}
static ssize_t bq24157_store_registers(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int ret;
	unsigned int reg;
	unsigned int val;

	ret = sscanf(buf, "%x %x", &reg, &val);
	if (ret == 2 && reg < 0x06)
		bq24157_write_byte((unsigned char)reg, (unsigned char *)&val,2);

	return count;
}

static DEVICE_ATTR(registers, 0644, bq24157_show_registers,
		   bq24157_store_registers);

static struct attribute *bq24157_attributes[] = {
	&dev_attr_registers.attr, NULL,
};

static const struct attribute_group bq24157_attr_group = {
	.attrs = bq24157_attributes,
};

static int bq24157_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct bq24157_info *info = NULL;

	pr_info("[bq24157_driver_probe]\n");

//+add by hzb for ontim debug
        if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
        {
           return -EIO;
        }
//-add by hzb for ontim debug
	new_client = client;

	ret = bq24157_get_vender_code();

	if (ret != 2) {
		pr_err("%s: get vendor id failed\n", __func__);
		return -ENODEV;
	}

	ret=bq24157_get_pn();
	if (ret != 2) {
		pr_err("%s: get pn failed\n", __func__);
		return -ENODEV;
	}

	ret=bq24157_get_revision();
	if (ret != 0x01) {
		pr_err("%s: get revision failed\n", __func__);
		return -ENODEV;
	}

	bq24157_read_byte(5, &bq24157_reg[0], 1);
	pr_err("%s;[0x%x]=0x%x \n", __func__,5, bq24157_reg[0]);
	if ((bq24157_reg[0] & 0x80) == 0 ) {
		pr_err("%s: get reg failed\n", __func__);
		return -ENODEV;
	}
	
	info = devm_kzalloc(&client->dev, sizeof(struct bq24157_info), GFP_KERNEL);

	if (!info)
		return -ENOMEM;

	info->dev = &client->dev;
	ret = bq24157_parse_dt(info, &client->dev);

	if (ret < 0)
		return ret;

	/* Register charger device */
	info->chg_dev = charger_device_register(info->chg_dev_name,
		&client->dev, info, &bq24157_chg_ops, &info->chg_props);

	if (IS_ERR_OR_NULL(info->chg_dev)) {
		pr_err("%s: register charger device failed\n", __func__);
		ret = PTR_ERR(info->chg_dev);
		return ret;
	}


	/* bq24157_hw_init(); //move to charging_hw_xxx.c */
	info->psy = power_supply_get_by_name("charger");

	if (!info->psy) {
		pr_err("%s: get power supply failed\n", __func__);
		return -EINVAL;
	}

	bq24157_reg_config_interface(0x06, 0xac);	/* ISAFE = 1550mA, VSAFE = 4.4V */

	bq24157_reg_config_interface(0x00, 0xC0);	/* kick chip watch dog */
	bq24157_reg_config_interface(0x01, 0xb8);	/* TE=1, CE=0, HZ_MODE=0, OPA_MODE=0 */
	bq24157_reg_config_interface(0x05, 0x83);

	bq24157_reg_config_interface(0x04, 0x02);	

	bq24157_set_otg_pl(1);

       bq24157_reg_config_interface(0x02, 0x02);//cccv 3.5v

	bq24157_dump_register(info->chg_dev);

	gtimer_init(&info->otg_kthread_gtimer, info->dev, "otg_boost");
	info->otg_kthread_gtimer.callback = usbotg_gtimer_func;

	info->otg_boost_workq = create_singlethread_workqueue("otg_boost_workq");
	INIT_WORK(&info->kick_work, usbotg_boost_kick_work);
	info->polling_interval = 20;
	g_info = info;

	ret = sysfs_create_group(&info->dev->kobj, &bq24157_attr_group);
	if (ret)
		pr_err( "%s failed to register sysfs. err: %d\n", __func__,ret);

//+add by hzb for ontim debug
        REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
//-add by hzb for ontim debug

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id bq24157_of_match[] = {
	{.compatible = "halo,bq24157"},
	{},
};
#else
static struct i2c_board_info i2c_bq24157 __initdata = {
	I2C_BOARD_INFO("bq24157", (bq24157_SLAVE_ADDR_WRITE >> 1))
};
#endif

static struct i2c_driver bq24157_driver = {
	.driver = {
		.name = "bq24157",
#ifdef CONFIG_OF
		.of_match_table = bq24157_of_match,
#endif
		},
	.probe = bq24157_driver_probe,
	.id_table = bq24157_i2c_id,
};

static int __init bq24157_init(void)
{

	if (i2c_add_driver(&bq24157_driver) != 0)
		pr_info("Failed to register bq24157 i2c driver.\n");
	else
		pr_info("Success to register bq24157 i2c driver.\n");

	return 0;
}

static void __exit bq24157_exit(void)
{
	i2c_del_driver(&bq24157_driver);
}

module_init(bq24157_init);
module_exit(bq24157_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C bq24157 Driver");
MODULE_AUTHOR("Henry Chen<henryc.chen@mediatek.com>");
