#include "mainwindow.h"
#include "ui_mainwindow.h"

extern "C" {
#ifdef _WIN32
#include <Windows.h>
#else
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif
}
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  ui->statusbar->showMessage(
      "Copyright © 2023: Abdo Daood   < abdo.daood94@gmail.com >");
  ui->lblVersion->setText("V 0.2");
  ui->statusbar->addPermanentWidget(ui->lblVersion);
  ui->cbxSerials->setCurrentIndex(-1);
  Q_FOREACH (const QSerialPortInfo &serialPortInfo,
             QSerialPortInfo::availablePorts()) {
    ui->cbxSerials->addItem(serialPortInfo.portName());
  }

  serialport.setBaudRate(QSerialPort::Baud115200);
  serialport.setDataBits(QSerialPort::Data8);
  serialport.setParity(QSerialPort::NoParity);
  serialport.setStopBits(QSerialPort::OneStop);
  serialport.setFlowControl(QSerialPort::NoFlowControl);
}

MainWindow::~MainWindow() {
  if (serialport.isOpen())
    serialport.close();
  delete ui;
}
void MainWindow::on_btnConnect_clicked() {
  if (ui->cbxSerials->count() <= 0) {
    Console_msg("there is no serial port to open");
    return;
  }
  serialport.setPortName(ui->cbxSerials->currentText());
  if (serialport.isOpen()) {
    if (ui->btnConnect->text() == "Connected") {
      serialport.close();
      ui->gbxCommands->setEnabled(false);
      ui->btnConnect->setText("connect");
      ui->btnConnect->setStyleSheet(
          "background-color: rgb(205, 255, 255);color: rgb(0, 0, 0);");
      Console_msg_error("Serial Port is Disconnected");
      return;
    }
    Console_msg_error("Error: Serial Port is Open from another program \"" +
                      serialport.errorString() + "\".");
  } else {
    if (serialport.open(QSerialPort::ReadWrite)) {
      //      int fd = serialport.handle();
      //      struct serial_struct kernel_serial_settings;
      //      ::ioctl(fd, TIOCGSERIAL, &kernel_serial_settings);
      //      kernel_serial_settings.flags |= ASYNC_LOW_LATENCY;
      //      ::ioctl(fd, TIOCSSERIAL, &kernel_serial_settings);

      Console_msg("Serial Port is Open");
      ui->btnConnect->setStyleSheet(
          "background-color: rgb(51, 209, 122);color: rgb(0, 0, 0);");
      Console_msg(">>> Connecting to STM32 >>>");
      ui->btnConnect->setStyleSheet(
          "background-color: rgb(51, 209, 122);color: rgb(0, 0, 0);");
      ui->btnConnect->setText("Connected");
      open_connection_with_stm32();
      return;
    }
    Console_msg_error("Error while Open serial port \"" +
                      serialport.errorString() + "\"");
  }
}

void MainWindow::open_connection_with_stm32() {
  BL_STM32_CMD_ stm_cmd = {.sof = BL_SOF,
                           .packet_type = BL_PACKET_TYPE_CMD,
                           .data_len = 1,
                           .cmd = BL_CMD_STAY_IN_BOOTLOADER_MODE,
                           .crc = 0x00,
                           .eof = BL_EOF};
  stm_cmd.crc = stm_crc32.CalcCRC((uint8_t *)&stm_cmd,
                                  5); /* 5= sof,packet_type,data_len,cmd*/
  BL_STM32_RESPONSE_ stm_resp = {.sof = 0,
                                 .packet_type = 0,
                                 .data_len = 0,
                                 .status = 0,
                                 .crc = 0,
                                 .eof = 0};
  // try to connect with MCU, and send frame to it every BL_MCU_OPEN_CTN_LOOP
  while (serialport.isOpen()) {
    serialport.write((char *)&stm_cmd, sizeof(stm_cmd));
    serialport.flush();
    ui->txtConsole->insertPlainText(" . ");
    if (readBytes((uint8_t *)&stm_resp, sizeof(stm_resp), BL_MCU_OPEN_CTN_LOOP,
                  UART_READ_SLEEP_US) == sizeof(stm_resp)) {
      if (stm_resp.sof == BL_SOF &&
          stm_resp.packet_type == BL_PACKET_TYPE_RESPONSE &&
          stm_resp.data_len == 1 && stm_resp.status == BL_ACK &&
          stm_resp.crc ==
              stm_crc32.CalcCRC((uint8_t *)&stm_resp,
                                5) && // 5= sof,packet_type,data_len,cmd
          stm_resp.eof == BL_EOF) {
        Console_msg_result(" STM32 in Bootloader Mode.");
        ui->gbxCommands->setEnabled(true);
        break;
      }
    }
    qApp->processEvents();
  }
}

void MainWindow::on_btnRefrech_2_clicked() {
  ui->cbxSerials->clear();
  Q_FOREACH (const QSerialPortInfo &serialPortInfo,
             QSerialPortInfo::availablePorts()) {
    ui->cbxSerials->addItem(serialPortInfo.portName());
  }
}
// print text inside EditePlainText widget  text with Red Color
void MainWindow::Console_msg_error(QString qstring) {
  QString combineHtml = "<p>";
  combineHtml.append(QString("<span style = 'color: red'>") + QString(qstring) +
                     QString("</span>"));
  combineHtml.append("</p>");
  ui->txtConsole->appendHtml(combineHtml);
  qApp->processEvents();
}
// Print Normal Black Line inside EditePlainText widget
void MainWindow::Console_msg(QString qstring) {
  ui->txtConsole->appendPlainText(qstring);
  qApp->processEvents();
}

// print text inside EditePlainText widget  text with Green Color
void MainWindow::Console_msg_result(QString qstring) {
  QString combineHtml = "<p>";
  combineHtml.append(QString("<span style = 'color: green'>") +
                     QString(qstring) + QString("</span>"));
  combineHtml.append("</p>");

  ui->txtConsole->appendHtml(combineHtml);
  qApp->processEvents();
}

void MainWindow::delay(uint32_t us) {
#ifdef _WIN32
  // Sleep(ms);
  __int64 time1 = 0, time2 = 0, freq = 0;

  QueryPerformanceCounter((LARGE_INTEGER *)&time1);
  QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

  do {
    QueryPerformanceCounter((LARGE_INTEGER *)&time2);
  } while ((time2 - time1) < us);
#else
  usleep(us);
#endif
}

void MainWindow::on_btnLunchApp_clicked() {
  if (serialport.isOpen()) {
    BL_STM32_CMD_ stm_cmd = {.sof = BL_SOF,
                             .packet_type = BL_PACKET_TYPE_CMD,
                             .data_len = 1,
                             .cmd = BL_CMD_LUNCH_APK,
                             .crc = 0x00,
                             .eof = BL_EOF};
    stm_cmd.crc = stm_crc32.CalcCRC((uint8_t *)&stm_cmd,
                                    5); /* 5= sof,packet_type,data_len,cmd*/
    Console_msg(">>> Launch Application >>>");
    serialport.write((char *)&stm_cmd, sizeof(stm_cmd));
    serialport.flush();
    switch (Receive_Response()) {
    case BL_ACK:
      Console_msg_result(" STM32: the Application is Launched.");
      break;
    case BL_NACK:
      Console_msg_error(" STM32: didn't Launch the Application.");
      break;
    default:
      return;
      break;
    }
    return;
  }
  Console_msg_error(" Error: Serial Port is Closed.");
}

void MainWindow::on_btnResetMCU_clicked() {
  if (serialport.isOpen()) {
    BL_STM32_CMD_ stm_cmd = {.sof = BL_SOF,
                             .packet_type = BL_PACKET_TYPE_CMD,
                             .data_len = 1,
                             .cmd = BL_CMD_RESET,
                             .crc = 0x00,
                             .eof = BL_EOF};
    stm_cmd.crc = stm_crc32.CalcCRC((uint8_t *)&stm_cmd,
                                    5); /* 5= sof,packet_type,data_len,cmd*/
    Console_msg(">>> Reset MCU >>>");
    serialport.write((char *)&stm_cmd, sizeof(stm_cmd));
    serialport.flush();
    switch (Receive_Response()) {
    case BL_ACK:
      Console_msg_result(" STM32: Software Reset is Done.");
      break;
    case BL_NACK:
      Console_msg_error(" STM32: Unable to Reset.");
      break;
    default:
      break;
    }
    return;
  }
  Console_msg_error(" Error: Serial Port is Closed.");
}

void MainWindow::on_btnGetBLVersion_clicked() {
  if (serialport.isOpen()) {
    BL_STM32_CMD_ stm_cmd = {.sof = BL_SOF,
                             .packet_type = BL_PACKET_TYPE_CMD,
                             .data_len = 1,
                             .cmd = BL_CMD_BL_VERSION,
                             .crc = 0x00,
                             .eof = BL_EOF};
    stm_cmd.crc = stm_crc32.CalcCRC((uint8_t *)&stm_cmd,
                                    5); /* 5= sof,packet_type,data_len,cmd*/
    Console_msg(">>> Get BootLoader Version >>>");
    serialport.write((char *)&stm_cmd, sizeof(stm_cmd));
    serialport.flush();
    uint8_t ret = Receive_Response();
    switch (ret) {
    case 0xFE:
      break;
    default:
      QString version = QString::number((ret & 0xF0) >> 4) + "." +
                        QString::number((ret & 0x0F));
      Console_msg_result(" STM32: Boolloader Version " + version);
      break;
    }
    return;
  }
  Console_msg_error(" Error: Serial Port is Closed.");
}

void MainWindow::on_btnFile_clicked() {

  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Open Binary file"), "", tr("Binary Files (*.bin)"));
  QFile binyfile(fileName);
  Console_msg("Opening Binary file ..");
  if (!binyfile.open(QIODevice::ReadOnly)) {
    Console_msg_error("Could not open the file for reading!!.");
    return;
  }
  fw_size = binyfile.size();
  if (fw_size > MAX_STM32_FW_SIZE) {
    Console_msg_error(
        "Error : Can not open File Whose size is more than 1MB !!");
    binyfile.close();
    return;
  }
  binyfile.seek(0);
  ui->txtFilepath->setText(fileName);
  ui->txtConsole->insertPlainText("\tDone.");
  Console_msg("File size = " + QString::number(fw_size) + "   Bytes.");
  for (uint32_t i = 0; i < fw_size; i++) {
    binyfile.read((char *)&fw_data[i], sizeof(char));
  }
  binyfile.close();
}

void MainWindow::on_btnFlash_clicked() {
  if (!serialport.isOpen()) {
    Console_msg_error(" Error: Serial Port is Closed.");
    return;
  }
  if (fw_size <= 0 || ui->txtFilepath->text() == NULL) {
    Console_msg_error(" Error : please, Upload a Valid Binary File.");
    return;
  }
  // 1- Send Header packet : size of binary file + CRC
  BL_FIRMWARE_INFO_ binary_info = {
      .firmware_size = fw_size,
      .firmware_crc = stm_crc32.CalcCRC((uint8_t *)fw_data, fw_size),
      .reserved1 = 0,
      .reserved2 = 0};
  BL_STM32_Header_ header = {.sof = BL_SOF,
                             .packet_type = BL_PACKET_TYPE_HEADER,
                             .data_len = sizeof(BL_FIRMWARE_INFO_),
                             .data = binary_info,
                             .crc = 0x00,
                             .eof = BL_EOF};
  header.crc =
      stm_crc32.CalcCRC((uint8_t *)&header,
                        4 + header.data_len); // 4= sof,packet_type,data_len
  serialport.write((char *)&header, sizeof(BL_STM32_Header_));
  serialport.flush();
  qDebug() << "Sending File Info  'Size,CRC'   :";
  // 2- wait Ack response
  switch (Receive_Response()) {
  case BL_ACK:
    Console_msg("Sending File Info  'Size,CRC'   : Done");
    break;
  case BL_NACK:
    Console_msg("Sending File Info  'Size,CRC'   : Error 'NACK.'");
    Console_msg_error("Downloading Stopped !!!");
    return;
    break;
  default:
    return;
    break;
  }
  qDebug() << "Done";
  qApp->processEvents();
  // 3- Send the Binary file  as Sequence of chunks & wait Ack For each chunk.
  BL_FRAME_TEMPLATE_ chunk = {.sof = BL_SOF,
                              .packet_type = BL_PACKET_TYPE_DATA,
                              .data_len = 0,
                              .data = &fw_data[0],
                              .crc = 0x00,
                              .eof = BL_EOF};
  uint8_t num = 0;
  uint8_t den = (fw_size / BL_DATA_MAX_SIZE) +
                ((fw_size % BL_DATA_MAX_SIZE == 0) ? 0 : 1);

  for (uint32_t i = 0; i < fw_size;) {
    (i + BL_DATA_MAX_SIZE <= fw_size) ? (chunk.data_len = BL_DATA_MAX_SIZE)
                                      : (chunk.data_len = fw_size - i);
    chunk.crc =
        stm_crc32.CalcCRC((uint8_t *)&chunk, 4); // 4= sof,packet_type,data_len
    chunk.crc =
        stm_crc32.CalcCRC_Add(chunk.crc, (uint8_t *)&chunk.data[i],
                              chunk.data_len); // 4= sof,packet_type,data_len
    serialport.write((char *)&chunk,
                     4); /*sof=1B, packet_type=1B, data_len=2B */
    serialport.write((char *)&chunk.data[i], chunk.data_len);
    serialport.write((char *)&chunk.crc, 5); /*send crc=4B > eof=1B*/
    serialport.flush();

    /* this is the first chunk, SO the mcu need some time to full erase
     * Flash apk sectors (5,6,7,8,9,10,11 in case of STM32F412ZGT6).
     */

    if (i == 0) {
      Console_msg("Erase App sectors in the Flash");
      uint8_t j = 5;
      while (j--) {
        WAIT_FLASH_ERASE_TIME
        ui->txtConsole->insertPlainText(" . ");
        qApp->processEvents();
      }
    }
    ///* However, i added and impliced this time inside  UART_READ_TIMEOUT_MS */

    i += chunk.data_len;
    QString msg =
        "[" + QString::number(++num) + "/" + QString::number(den) + "]";
    qDebug() << msg + "?";
    switch (Receive_Response()) {
    case BL_ACK:
      Console_msg(msg + "\tDone");
      break;
    case BL_NACK:
      Console_msg(msg + "\tError 'NACK.'");
      Console_msg_error("Downloading Stopped !!!");
      return;
      break;
    default:
      Console_msg_error("Downloading Stopped !!!");
      return;
      break;
    } // switch loop
  }   // for loop
  // 4- send End Command to let MCU Verify all the binary file
  BL_STM32_CMD_ stm_cmd = {.sof = BL_SOF,
                           .packet_type = BL_PACKET_TYPE_CMD,
                           .data_len = 1,
                           .cmd = BL_CMD_VERIFY,
                           .crc = 0x00,
                           .eof = BL_EOF};
  stm_cmd.crc = stm_crc32.CalcCRC((uint8_t *)&stm_cmd,
                                  5); /* 5= sof,packet_type,data_len,cmd*/
  QString msg = "Verify Binary Data: >>>";
  serialport.write((char *)&stm_cmd, sizeof(stm_cmd));
  serialport.flush();
  switch (Receive_Response()) {
  case BL_ACK:
    Console_msg_result(msg + "\tDone");
    break;
  case BL_NACK:
    Console_msg(msg + "\tError 'NACK.'");
    Console_msg_error("Flashing Failed !!!");
    return;
    break;
  default:
    Console_msg_error("Flashing Failed !!!");
    return;
    break;
  } // switch loop
}

uint8_t MainWindow::Receive_Response() {
  qApp->processEvents();
  BL_STM32_RESPONSE_ stm_resp = {.sof = 0,
                                 .packet_type = 0,
                                 .data_len = 0,
                                 .status = 0,
                                 .crc = 0,
                                 .eof = 0};
  if (readBytes((uint8_t *)&stm_resp, sizeof(stm_resp), UART_READ_TIMEOUT_MS,
                UART_READ_SLEEP_US) == sizeof(stm_resp)) {
    if (stm_resp.sof == BL_SOF &&
        stm_resp.packet_type == BL_PACKET_TYPE_RESPONSE &&
        stm_resp.data_len == 1 &&
        stm_resp.crc ==
            stm_crc32.CalcCRC(
                (uint8_t *)&stm_resp,
                4 + stm_resp.data_len) // 4= sof,packet_type,data_len
        && stm_resp.eof == BL_EOF) {
      return stm_resp.status;
    }
    Console_msg_error(" Error : Response with uncorrected CRC.");
    return 0xFE; // CRC Error
  }
  Console_msg_error(" Error: Timout while waiting MCU response.");
  return 0xFE; // Error of Timout  ,
}

// Readbytes from UART with Timeout feature
int MainWindow::readBytes(uint8_t *buffer, unsigned int maxNbBytes,
                          unsigned int timeOut_ms,
                          unsigned int sleepDuration_us) {
  timeOut timer;
  // set the instant time into praivate variable
  timer.initTimer();
  unsigned int NbByteRead = 0;
  // While Timeout is not reached
  while (timer.elapsedTime_ms() < timeOut_ms || timeOut_ms == 0) {
    qApp->processEvents();
    int Ret =
        serialport.read((char *)buffer + NbByteRead, maxNbBytes - NbByteRead);
    // Error while reading
    if (Ret == -1)
      return -2;

    // One or several byte(s) has been read on the device
    if (Ret > 0) {
      NbByteRead += Ret;
      // Success : bytes has been read
      if (NbByteRead >= maxNbBytes)
        return NbByteRead;
    }
    // Suspend the loop to avoid charging the CPU
    usleep(sleepDuration_us);
    qApp->processEvents();
  }
  // Timeout reached, return the number of bytes read
  return NbByteRead;
}
