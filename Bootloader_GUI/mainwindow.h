#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// My Headers
#include "bootloader_update.h"
#include "timeout.h"
#include "stm32f412_crc32.h"
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include <QTextStream>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_btnConnect_clicked();
  void on_btnRefrech_2_clicked();
  void Console_msg(QString);
  void Console_msg_result(QString);
  void Console_msg_error(QString);
  void delay(uint32_t);
  int readBytes(uint8_t *buffer, unsigned int maxNbBytes, unsigned int timeOut_ms,
                unsigned int sleepDuration_us);
  void open_connection_with_stm32(void);
  uint8_t Receive_Response();
  void on_btnLunchApp_clicked();
  void on_btnResetMCU_clicked();
  void on_btnGetBLVersion_clicked();
  void on_btnFile_clicked();
  void on_btnFlash_clicked();

private:
  Ui::MainWindow *ui;
  QSerialPort serialport;
  STM32F412_CRC32 stm_crc32;
  uint32_t fw_size;
  uint8_t fw_data[MAX_STM32_FW_SIZE];

  typedef struct {
    uint32_t firmware_size;
    uint32_t firmware_crc;
    uint32_t reserved1;
    uint32_t reserved2;
  } __attribute__((packed)) BL_FIRMWARE_INFO_;



 /*
 * Header Frame format
 *  ________________________________________________________________
 *  | SOF	|	Packet Type	|	Len		|	type	|	CRC	|	EOF	|
 *  |_______|_______________|___________|___________|_______|_______|
 *  | 1B	|		1B		|	2B		|	 16B	|	4B	 	1B
 *  ^----------------------CRC----------------------^
 */
  typedef struct {
    uint8_t sof;
    uint8_t packet_type;
    uint16_t data_len;
    BL_FIRMWARE_INFO_ data;
    uint32_t crc;
    uint8_t eof;
  } __attribute__((packed)) BL_STM32_Header_;


  /*
 * Response Frame format
 *  ________________________________________________________________
 *  | SOF	|	Packet Type	|	Len		|	type	|	CRC	|	EOF	|
 *  |_______|_______________|___________|___________|_______|_______|
 *  | 1B	|		1B		|	2B		|	 1B		|	4B	 	1B
 *  ^----------------------CRC----------------------^
 */
  typedef struct {
    uint8_t sof;
    uint8_t packet_type;
    uint16_t data_len;
    uint8_t status;
    uint32_t crc;
    uint8_t eof;
  } __attribute__((packed)) BL_STM32_RESPONSE_;


  typedef struct {
    uint8_t sof;
    uint8_t packet_type;
    uint16_t data_len;
    uint8_t cmd;
    uint32_t crc;
    uint8_t eof;
  } __attribute__((packed)) BL_STM32_CMD_;


  /*
 *  Standard & General Frame format
 *  ----------------------------------------------------------------
 *  | SOF	|	Packet Type	|	Len		|	Data	|	CRC	|	EOF	|
 *  |		|	 			|			|			|		|  		|
 *  | 1B	|		1B		|	2B		|	n*B		|	4B	| 	1B	|
 *  ----------------------------------------------------------------
 *  ^----------------------CRC----------------------^
 *  */

  typedef struct {
    uint8_t sof;
    uint8_t packet_type;
    uint16_t data_len;
    uint8_t *data;
    uint32_t crc;
    uint8_t eof;

  } __attribute__((packed)) BL_FRAME_TEMPLATE_;
};
#endif // MAINWINDOW_H
