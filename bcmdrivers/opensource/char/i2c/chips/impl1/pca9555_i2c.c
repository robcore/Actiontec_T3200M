/*
   Copyright (c) 2015 Actiontec
   All Rights Reserved
   Wrote by Ken.S
*/

#include <linux/version.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>  /* kzalloc() */
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>

/* PCA9555 I2C client chip addresses */
#define PCA9555_I2C_ADDR1 0x24 

#define PCA9555_INPUT           0
#define PCA9555_OUTPUT          1
#define PCA9555_INVERT          2
#define PCA9555_DIRECTION       3

/*  
 *  T3000 GPIO Pin used as below:
 *  Board ID : 0x08  IO0_0~IO0_7(bit0~7)    input   
 *  VOIP_LINE1_GRN , VOIP LED 1 :  IO1_0(bit8)     output;   active high
 *  VOIP_LINE2_GRN , VOIP LED 2 :  IO1_1(bit9)     output;   active high
 *  RNC_CNTRL ,  RCN control : IO1_4(bit 12)       output;  active high
 *  RELAY_CNTRL, Relay control : IO1_5(bit13)    output;  active high
 *  bit 0~7: IO0, bit 8-15: IO1
*/
#define AEI_PIN_DIR 0xCCFF   // 1 - input , 0 - output
#define AEI_PIN_POLARITY 0x0 // 1 - Inverted., 0 - Retained
#define AEI_PIN_OUTPUT 0xE000   // 1 - High., 0 - Low, VOIP LED- low, RCN- low, RELAY-High

uint16_t reg_input;    
uint16_t reg_output;
uint16_t reg_direction;
uint16_t reg_polarity;	
static spinlock_t pca9555_spinlock;
    
/* Addresses to scan */
static unsigned short normal_i2c[] = {PCA9555_I2C_ADDR1, I2C_CLIENT_END};

struct i2c_client *globalc;

static int pca9555_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int pca9555_i2c_remove(struct i2c_client *client);
static int pca9555_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);

static const struct i2c_device_id pca9555_i2c_id_table[] = {
    { "pca9555", 0 },
    { },
};
MODULE_DEVICE_TABLE(i2c, pca9555_i2c_id_table);

static struct i2c_driver pca9555_i2c_driver = {
    .class = ~0,
    .driver = {
        .name = "pca9555",
    },
    .probe  = pca9555_i2c_probe,
    .remove = pca9555_i2c_remove,
    .id_table = pca9555_i2c_id_table,
    .detect  = pca9555_i2c_detect,
    .address_list = normal_i2c

};


static int pca9555_write_reg(struct i2c_client *client, int reg, uint16_t val)
{
    int ret = 0;
    ret = i2c_smbus_write_word_data(client,reg << 1, val);

    if (ret < 0) {
        printk(" pca9555_write_reg failed writing register ret=%d\n",ret);
        return ret;
    }

    return 0;
}

static int pca9555_read_reg(struct i2c_client *client, int reg, uint16_t *val)
{
    int ret;
    ret = i2c_smbus_read_word_data(client, reg << 1);
   
    if (ret < 0) {
        printk(" pca9555_write_reg failed reading register ret=%d\n",ret);
        return ret;
    }

    *val = (uint16_t)ret;
    return 0;
}

/*
 * offset: bit 0~7: IO1, bit 8-15: IO0
 * value : 1 - High, 0 - Low
*/
static int pca9555_set_output_value(uint32_t offset, int val)
{
    int ret = 0;

    printk("set reg_output: = %x, offset=%u,value=%d\n",reg_output,offset,val);
    spin_lock_bh(&pca9555_spinlock);   
    /* Write OUPUT value */
    if (val)
        reg_output |= (1u << offset);
    else
        reg_output &= ~(1u << offset);
    printk("after set reg_output: = %x, offset=%u,value=%d\n",reg_output,offset,val);
    pca9555_write_reg(globalc, PCA9555_OUTPUT,reg_output);
    spin_unlock_bh(&pca9555_spinlock);   
    return ret;
}
EXPORT_SYMBOL(pca9555_set_output_value);


static int pca9555_get_output_value(uint32_t offset)
{

    printk("get reg_output: = %x, offset=%u\n",reg_output,offset);
    
    return (reg_output & (1u << offset)) ? 1 : 0;
     
}
EXPORT_SYMBOL(pca9555_get_output_value);

static unsigned int pca9555_get_output_all_value(void)
{

    //printk("get reg_output: = %x\n",reg_output);

    return reg_output;

}
EXPORT_SYMBOL(pca9555_get_output_all_value);


static int pca9555_get_input_value(void)
{

    //printk("get reg_input: = %x, offset=%u\n",reg_input,offset);
    
    return reg_input;
     
}
EXPORT_SYMBOL(pca9555_get_input_value);


static int __devinit pca9555_init(struct i2c_client *client)
{
    int ret;

    /* Read INPUT value */
    ret = pca9555_read_reg(client, PCA9555_INPUT, &reg_input);
    if (ret)
        goto out;
    printk("reg_input = %x\n",reg_input);
   
    /* Write OUPUT value */
    reg_output = AEI_PIN_OUTPUT;
    pca9555_write_reg(client, PCA9555_OUTPUT,reg_output);
 
    /* Read OUPUT value */
    ret = pca9555_read_reg(client, PCA9555_OUTPUT, &reg_output);
    if (ret)
        goto out;
    printk("reg_output = %x\n",reg_output);
    
    /* Write POLARITY INVERSION value */
    reg_polarity = AEI_PIN_POLARITY;
    pca9555_write_reg(client, PCA9555_INVERT,reg_polarity);

    /* Read POLARITY INVERSION value */ 
    ret = pca9555_read_reg(client, PCA9555_INVERT,&reg_polarity);	
    if (ret)
        goto out;   
    printk("reg_polarity = %x\n",reg_polarity);   
         
    /* Write DIRECTION value */           
    reg_direction = AEI_PIN_DIR;
    pca9555_write_reg(client, PCA9555_DIRECTION,reg_direction);
    
    /* Read DIRECTION value */     
    ret = pca9555_read_reg(client, PCA9555_DIRECTION,&reg_direction);
    if (ret)
        goto out;
    printk("reg_direction = %x\n",reg_direction);   
out:
    return ret;
}

static int pca9555_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int err = 0;

    globalc = client;
    err = pca9555_init(client);
    
    return err;
}

static int pca9555_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    strcpy(info->type, "pca9555");
    info->flags = 0;
    return 0;
}

static int pca9555_i2c_remove(struct i2c_client *client)
{
    int err = 0;
    kfree(i2c_get_clientdata(client));
    return err;
}

static int pca9555_i2c_add_driver(void)
{
    int ret;

    ret = i2c_add_driver(&pca9555_i2c_driver);
    if (ret){
	printk("ret =%d, i2c_add_drivern",ret);
        return ret;
    }

    /* no PCA9555 I2C device presented at bus */
    if (globalc == NULL)
    {
	printk("no PCA9555 I2C device presented at bus");
        i2c_del_driver(&pca9555_i2c_driver);
        return -1;
    }
    printk("PCA9555 I2C Module AEI Driver Loaded\n");
    return 0;
}

static int proc_aei_read_cmd(char *page,char **stat,off_t off,int count,int *eof,void *data)
{
    int len = 0;
    len = sprintf(page, "OUTPUT = %x, INPUT = %x\n",pca9555_get_output_all_value(),pca9555_get_input_value());
    return len;
}

/*
 * This function expect input in the form of:
 * echo "xxyy" > /proc/pca9555/output
 * where xx is hex for the Pin num, 08-VOIP LED1, 09-VOIP LED2, 0c-RNC, 0d- RELAY
 * and   yy is hex for the level. 00-off, 01-on
 * For example,
 *     echo "0801" > //proc/pca9555/output - turn on VOIP LED1
 *
 */   
static int proc_aei_write_cmd(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
    unsigned int gpiodata;
    char input[32];
    int i;

    if (count > 31)
        count = 31;

    if (copy_from_user(input, buffer, count) != 0)
        return -EFAULT;


    for (i = 0; i < count; i ++)
    {
        if (!isxdigit(input[i]))
        {
            input[i] = 0;
        }
    }
    input[i] = 0;

    if (0 != kstrtouint(input, 16, &gpiodata))
        return -EFAULT;

    /* get the PATCH ID */
    printk("pin =%x, value =%x\n",(gpiodata & 0xff00) >> 8,gpiodata & 0xff);
    pca9555_set_output_value((gpiodata & 0xff00) >> 8,gpiodata & 0xff);
    return count;
}

int pca9555_proc_init(void)
{
    struct proc_dir_entry *pca9555_proc_root = NULL;
    struct proc_dir_entry *pca9555_proc_output = NULL;	
    unsigned int id = 0;
    /* Create the /proc/pca9555 entry */
    pca9555_proc_root = proc_mkdir("pca9555", NULL);
    if (!pca9555_proc_root)
       return -ENOMEM;

    pca9555_proc_output = create_proc_entry("output", S_IRUGO | S_IWUSR,pca9555_proc_root);   
    if(pca9555_proc_output != NULL ){
            pca9555_proc_output->write_proc = proc_aei_write_cmd;
            pca9555_proc_output->read_proc = proc_aei_read_cmd;
            pca9555_proc_output->data = (void*)id;

     }
 
   return 0;
}

static int __init pca9555_i2c_init(void)
{
    pca9555_i2c_add_driver();
    spin_lock_init(&pca9555_spinlock);
    pca9555_proc_init();
    return 0;
}


static void __exit pca9555_i2c_exit(void)
{
    i2c_del_driver(&pca9555_i2c_driver);
}

module_init(pca9555_i2c_init);
module_exit(pca9555_i2c_exit);



