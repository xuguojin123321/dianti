#ifndef  __BSP_MCU_CRYPTOGRAPHY_H__
#define  __BSP_MCU_CRYPTOGRAPHY_H__

#include <stdint.h>
#include <stdbool.h>


typedef int		int32_t;

/*===============================AES加密、解密相关定义 start========================================*/
// 以bit为单位的密钥长度，只能为 128，192 和 256 三种
#define AES_KEY_LENGTH	128

// 加解密模式
#define AES_MODE_ECB	0				// 电子密码本模式（一般模式）
#define AES_MODE_CBC	1				// 密码分组链接模式
#define AES_MODE		AES_MODE_CBC

//	函数名：	aes_init
//	描述：		初始化，在此执行扩展密钥操作。
//	输入参数：	pKey -- 原始密钥，其长度必须为 AES_KEY_LENGTH/8 字节。
//	输出参数：	无。
//	返回值：	无。
///
void aes_init(const void *pKey);

//
//	函数名：	aes_encrypt
//	描述：		加密数据
//	输入参数：	pPlainText	-- 明文，即需加密的数据，其长度为nDataLen字节。
//				nDataLen	-- 数据长度，以字节为单位，必须为AES_KEY_LENGTH/8的整倍数。
//				pIV			-- 初始化向量，如果使用ECB模式，可设为NULL。
//	输出参数：	pCipherText	-- 密文，即由明文加密后的数据，可以与pPlainText相同。
//	返回值：	无。
//
void aes_encrypt(const unsigned char *pPlainText, unsigned char *pCipherText, 
				 unsigned int nDataLen, const unsigned char *pIV);

//
//	函数名：	aes_decrypt
//	描述：		解密数据
//	输入参数：	pCipherText -- 密文，即需解密的数据，其长度为nDataLen字节。
//				nDataLen	-- 数据长度，以字节为单位，必须为AES_KEY_LENGTH/8的整倍数。
//				pIV			-- 初始化向量，如果使用ECB模式，可设为NULL。
//	输出参数：	pPlainText  -- 明文，即由密文解密后的数据，可以与pCipherText相同。
//	返回值：	无。
//
void aes_decrypt( const unsigned char *pCipherText,unsigned char *pPlainText,unsigned int nDataLen, const unsigned char *pIV);

/*===============================AES加密、解密相关定义 end========================================*/
/*用法示例：
AES加密在线工具：https://lzltool.com/AES
使用注意点：
1、AES_KEY_LENGTH取值只能是128，192 和 256
2、密钥和向量表长度为AES_KEY_LENGTH/8个字节
3、加密、解密数据长度为AES_KEY_LENGTH/8的整数倍字节
u8 buf[16],saveBuf[16],descryptBuf[16];
	u16 i;
	unsigned char AES128key[16] = "123456789abcdefa";//秘钥
	unsigned char AES_IV[16]= "0102030405123456";//向量表
	delay_init(168);		  //初始化延时函数
	LED_Init();		        //初始化LED端口

	aes_init(AES128key);//AES初始化
	
	for(i=0;i<sizeof(buf);i++)
	{
		buf[i]=i;
	}
	while(1)
	{
		aes_encrypt(buf,saveBuf,sizeof(buf), AES_IV);
		aes_decrypt(saveBuf,descryptBuf, sizeof(buf), AES_IV);
		delay_ms(500);                     //延时300ms
	}
*/
/*===============================MD5相关定义 end========================================*/

#endif	//__BSP_MCU_CRYPTOGRAPHY_H__

