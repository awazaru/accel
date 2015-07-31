/*ppppppp*/
/*3軸加速度センサ実験用ソースコード*/
/*Atmega328P*/
#include <avr/io.h>
#include <math.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define X_G    517//Xオフセット 感度6G時
#define Y_G    525//Yオフセット
#define Z_G    520//Zオフセット
#define G_unit  41//1G単位

#define FOSC   1000000//動作周波数
#define BAUD   2400//ボーレート
#define MYUBRR (FOSC/16/BAUD)-1

// ｼﾘｱﾙ通信ｺﾏﾝﾄﾞ（ｱｽｷｰｺｰﾄﾞ)
#define LF        usart_tx(0x0a)        //改行
#define CR        usart_tx(0x0d)        //復帰
#define HT        usart_tx(0x09)        //水平ﾀﾌﾞ
#define is_SET(x,y) ((x) & (1<<(y)))


/*変数定義*/
uint8_t i=1;//x,y,z切り替え
uint8_t shuffle = 0;//振られたことを検知するフラグ
uint8_t shuffle_count =0;//振られた回数をカウント
float xg_data=0,yg_data=0,zg_data=0;
int16_t x_data=0,y_data=0,z_data=0;
float xyz_data=0;
float dxy_data=0;
int8_t x_rad=0,y_rad=0;//角度

/*関数宣言*/
void usart_tx(unsigned char data){//送信用関数
  while( !(UCSR0A & (1<<UDRE0)) );        //送信ﾊﾞｯﾌｧ空き待機
  UDR0 = data;
}

unsigned char usart_rx(void){//受信用関数
  while( !(UCSR0A & (1<<RXC0)) );                //受信完了待機
  return UDR0;                                //受信ﾃﾞｰﾀ返す
}


void puts_tx(char *str){//文字列送信用関数
  while( *str != 0x00 ){ //nullになるまで実行
    usart_tx(*str);
    str++;                                    //ｱﾄﾞﾚｽ+1
  }
}

void serial_ini(){// シリアル通信設定
  UBRR0 = MYUBRR;
  UCSR0A=0b00000000;//受信すると10000000 送信有効になると00100000
  UCSR0B|=0b00011000;//送受信有効
  UCSR0C=0b00000110;//データ8bit、非同期、バリティなし、stop1bit
}

void print_tx(int val)    //PWMのﾃﾞｰﾀ＆電圧値換算し送信
{
  if(val<0){
    val=-val;
    puts_tx("-");
  }
  unsigned char a4,a3,a2,a1;        //PWMのﾃﾞｰﾀ各桁
    
  a4 = (val/1000)+0x30;        //1000の位
  a3 = (val/100)%10+0x30;        //100の位
  a2 = (val/10)%10+0x30;        //10の位
  a1 = (val%10)+0x30;            //1の位
    
  usart_tx(a4);        //ﾃﾞｰﾀ各桁の送信
  usart_tx(a3);
  usart_tx(a2);
  usart_tx(a1);
    
  return;
}

void timer_ini(){//タイマー設定
    
    
}

void adc_ini(){//ADコンバーター設定
  //ADUMXはmain文for(;;)内にあります
  ADCSRA |=_BV(ADEN)|_BV(ADPS1);//ADC許可,割込み許可,4分周 ADSCを1でAD変換開始
  DIDR0 |=_BV(PIN0)|_BV(PIN1)|_BV(PIN2);//ADC0,1,2ピンデジタル入力禁止
}

void pin_ini(){
  DDRC=0;
  DDRD=0b10000000;//PD7
}

void debug(int ad_data){
  print_tx(ad_data);
  // _delay_ms(100);
}

int adc_convert(void){//AD変換及びシリアル通信
  if(i>=4)
    i=1;
  int adc_data;
  ADCSRA|= _BV(ADSC);//ADC開始
  while(is_SET(ADCSRA,ADIF)==0);  //変換終了まで待機
  adc_data=(ADCL>>6)+(ADCH<<2);
  switch (i) {
  case 1: puts_tx("X:");
    x_data = adc_data;
    x_data=x_data-X_G;//オフセット調整
    break;
  case 2: puts_tx("Y:");
    y_data = adc_data;
    y_data=y_data-Y_G;
    break;
  case 3: puts_tx("Z:");
    z_data = adc_data;
    z_data=z_data-Z_G;
    break;
  default:
    break;
  }
  debug(adc_data);
  i++;
  return 0;
}

/*メインルーチン*/
int main(void){
    
  serial_ini();
  adc_ini();
  pin_ini();
    
  sei();//allow interrupt
  ADCSRA|= _BV(ADSC);//ADC初期化　単独変換動作
    
  for(;;){
    ADMUX|=_BV(ADLAR)|_BV(MUX0);//Aref外部基準電圧　ADC1
    ADMUX&=~_BV(MUX1);
    adc_convert();//AD変換及び値代入
    HT;
    ADMUX|=_BV(ADLAR)|_BV(MUX1); //ADC2
    ADMUX&=~_BV(MUX0);
    adc_convert();//AD変換及び値代入
    HT;
    ADMUX|=_BV(ADLAR)|_BV(MUX1)|_BV(MUX0);//ADC3
    adc_convert();//AD変換及び値代入
    LF;
        
    /* x_rad=atan2(x_data,z_data)/M_PI*180.0;//角度
	  y_rad=atan2(y_data,z_data)/M_PI*180.0;
	  debug(x_rad);
	  HT;
	  debug(y_rad);
	  LF;
    */
        
    //ふりふりしてる間LED点灯
    xg_data=(x_data*10)/G_unit/9.8;
    yg_data=(y_data*10)/G_unit/9.8;
    zg_data=(z_data*10)/G_unit/9.8;
    xyz_data=sqrt(xg_data*xg_data+yg_data*yg_data+zg_data*zg_data);
    debug(xyz_data);//加速度を10倍にして表示
    if(xyz_data>=2.3){
	 shuffle =1;
	 puts_tx("accel");
    }else{
	 shuffle =0;
    }
    LF;
        
    if(shuffle==1){
	 PORTD=0b10000000;
    }else{
	 PORTD=0b00000000;
    }

  }
  return 0;
}
