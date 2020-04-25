
#include <stdio.h>
#include <unistd.h>
#include "bcm2835.h" // https://www.airspayce.com/mikem/bcm2835/

// GPIO pin for I2C
#define I2C_SCL_PIN RPI_V2_GPIO_P1_05
#define I2C_SDA_PIN RPI_V2_GPIO_P1_03

#define LCD_ADDRESS 0x27
#define LCD_CMD 0
#define LCD_CHR 1
#define LCD_BACKLIGHT 0x08
#define ENABLE 0x04

// software I2C
class I2C {
private:
	void delay()
	{
//		usleep(1);
	}

	// 初期化
	void init_i2c()
	{
		// SCLピンを入力モードにする
		bcm2835_gpio_fsel(I2C_SCL_PIN, BCM2835_GPIO_FSEL_INPT);
		// SDAピンを入力モードにする
		bcm2835_gpio_fsel(I2C_SDA_PIN, BCM2835_GPIO_FSEL_INPT);
		// SCLピンをLOWにする
		bcm2835_gpio_clr(I2C_SCL_PIN);
		// SDAピンをLOWにする
		bcm2835_gpio_clr(I2C_SDA_PIN);
	}

	void i2c_cl_0()
	{
		// SCLピンを出力モードにするとピンの状態がLOWになる
		bcm2835_gpio_fsel(I2C_SCL_PIN, BCM2835_GPIO_FSEL_OUTP);
	}

	void i2c_cl_1()
	{
		// SCLピンを入力モードにするとピンの状態がHIGHになる
		bcm2835_gpio_fsel(I2C_SCL_PIN, BCM2835_GPIO_FSEL_INPT);
	}

	void i2c_da_0()
	{
		// SDAピンを出力モードにするとピンの状態がLOWになる
		bcm2835_gpio_fsel(I2C_SDA_PIN, BCM2835_GPIO_FSEL_OUTP);
	}

	void i2c_da_1()
	{
		// SDAピンを入力モードにするとピンの状態がHIGHになる
		bcm2835_gpio_fsel(I2C_SDA_PIN, BCM2835_GPIO_FSEL_INPT);
	}

	int i2c_get_da()
	{
		// SDAピンの状態を読み取る
		return bcm2835_gpio_lev(I2C_SDA_PIN) ? 1 : 0;
	}

	// スタートコンディション
	void i2c_start()
	{
		i2c_da_0(); // SDA=0
		delay();
		i2c_cl_0(); // SCL=0
		delay();
	}

	// ストップコンディション
	void i2c_stop()
	{
		i2c_cl_1(); // SCL=1
		delay();
		i2c_da_1(); // SDA=1
		delay();
	}

	// リピーテッドスタートコンディション
	void i2c_repeat()
	{
		i2c_cl_1(); // SCL=1
		delay();
		i2c_da_0(); // SDA=0
		delay();
		i2c_cl_0(); // SCL=0
		delay();
	}

	// 1バイト送信
	bool i2c_write(int c)
	{
		int i;
		bool nack;

		delay();

		// 8ビット送信
		for (i = 0; i < 8; i++) {
			if (c & 0x80) {
				i2c_da_1(); // SCL=1
			} else {
				i2c_da_0(); // SCL=0
			}
			c <<= 1;
			delay();
			i2c_cl_1(); // SCL=1
			delay();
			i2c_cl_0(); // SCL=0
			delay();
		}

		i2c_da_1(); // SDA=1
		delay();

		i2c_cl_1(); // SCL=1
		delay();
		// NACKビットを受信
		nack = i2c_get_da();
		i2c_cl_0(); // SCL=0

		return nack;
	}

	int address; // I2Cデバイスアドレス

public:
	I2C(int address)
		: address(address)
	{
		init_i2c();
	}

	// デバイスのレジスタに書き込む
	virtual void write(int data)
	{
		i2c_start();                   // スタート
		i2c_write(address << 1);       // デバイスアドレスを送信
//		i2c_write(reg);                // レジスタ番号を送信
		i2c_write(data);               // データを送信
		i2c_stop();                    // ストップ
	}

};


I2C *wire;

void lcd_byte(uint8_t bits, uint8_t mode)
{
	uint8_t hi = mode | (bits & 0xf0) | LCD_BACKLIGHT;
	uint8_t lo = mode | ((bits << 4) & 0xf0) | LCD_BACKLIGHT;
	usleep(300);
	wire->write(hi);
	usleep(1);
	wire->write(hi | ENABLE);
	usleep(1);
	wire->write(hi);
	usleep(1);
	wire->write(lo);
	usleep(1);
	wire->write(lo | ENABLE);
	usleep(1);
	wire->write(lo);
	usleep(300);
}

void lcd_print(char const *ptr)
{
	while (*ptr) {
		lcd_byte(*ptr, LCD_CHR);
		ptr++;
	}
}

void lcd_clear()
{
	lcd_byte(0x01, LCD_CMD);
	usleep(500);
}

void setup()
{
	bcm2835_init();
	wire = new I2C(LCD_ADDRESS);
//	usleep(1000000);
	usleep(500);
	lcd_byte(0x33, LCD_CMD);
	lcd_byte(0x32, LCD_CMD);
	lcd_byte(0x06, LCD_CMD);
	lcd_byte(0x0c, LCD_CMD);
	lcd_byte(0x28, LCD_CMD);
	lcd_clear();

}


void loop()
{
	static int i = 0;
	char tmp[5];
	tmp[0] = (i / 1000) % 10 + '0';
	tmp[1] = (i / 100) % 10 + '0';
	tmp[2] = (i / 10) % 10 + '0';
	tmp[3] = (i / 1) % 10 + '0';
	tmp[4] = 0;
	lcd_clear();
	lcd_print(tmp);
	sleep(1);
	i++;
}

int main()
{
	setup();

	while (1) {
		loop();
	}

	bcm2835_close();

	return 0;
}

