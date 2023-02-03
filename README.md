# M253BSP_CAN_NoFilter
 M253BSP_CAN_NoFilter

update @ 2023/02/02

1. set CAN FD as CAN bus mode , and no filter ID , to receive message

2. enable define :ENABLE_LOOP_BACK , to test internal TX/RX function

3. key point 1: check eCANFD_ACC_NON_MATCH_FRM_RX_FIFO0 in CANFD_SetGFC setting

4. key point 2: check CANFD_ReadRxFifoMsg setting

5. use terminal digit 1/2/3/4 , to send TX message and recieve RX message under loop 

below is log message result

![image](https://github.com/released/M253BSP_CAN_NoFilter/blob/main/log.jpg)	
