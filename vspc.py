#!/usr/bin/python
#!coding:utf-8
from logging import exception
import os, time, pprint, shutil
import sys, serial, argparse 
import subprocess, tempfile, re 
import threading
pp = pprint.PrettyPrinter(indent=4)


# def sleep( timeout=10 ):
#     icon_index = 0
#     start_time = time.time()
#     iconList = ['-','\\','|','/']
#     old = 0
#     while True:        
#         new = time.time()
#         if new > (start_time + timeout):
#             break       
#         if (new - old) >= 0.5:
#             icon_index = (icon_index + 1)%len(iconList)
#             print( iconList[icon_index], end = '\r')          
#             old = new
#         time.sleep(0.1)
#     print('\n')


# def MultiThread_Test():

   

#     semaphore = threading.BoundedSemaphore(1) 



#     def record_test( times, delay, dev, file, sr, bit, ch):
#         start = time.time()
#         id = threading.currentThread().name
#         counter = 0
#         for i in range(times) :
#             print(f'[{id}][{i}]: record_test {i}th test <{counter}>' + '-'*5)
#             print(f"[{id}][{i}]: acquire  : {time.time()}")
#             semaphore.acquire()            
#             print(f"[{id}][{i}]: acquired : {time.time()}") 
#             a=  audio.record( dev, file.replace('.wav',f'_{i}.wav'), sr, bit, ch ) # dev[1], wavefile_record, 16000, 16, 2) )
#             semaphore.release()
#             if a != True:
#                 counter = counter + 1
#                 print(f'[{id}][{i}]: ret = {a}, error counter = {counter}','='*10)
#             time.sleep(delay)
#             semaphore.acquire() 
#             a = audio.stop_record( dev ) 
#             semaphore.release()            
#             if a != True:
#                 counter = counter + 1 
#                 print(f'[{id}][{i}]: ret = {a}, error counter = {counter}','='*10)
#             print(f'[{id}][{i}]: Time cost = {time.time() - start}\n')
         
    
#     def play_test( times, delay, dev, file):
#         start = time.time()
#         id = threading.currentThread().name
#         counter = 0
#         for i in range(times) :
#             print(f'[{id}][{i}]: record_test {i}th test <{counter}>' + '-'*5)
#             print(f"[{id}][{i}]: acquire  : {time.time()}")
#             semaphore.acquire()            
#             print(f"[{id}][{i}]: acquired : {time.time()}") 
#             a=  audio.play( dev, file)
#             semaphore.release()
#             if a != True:
#                 counter = counter + 1
#                 print(f'[{id}][{i}]: ret = {a}, error counter = {counter}','='*10)
#             time.sleep(delay)
#             semaphore.acquire() 
#             a = audio.stop_play( dev ) 
#             semaphore.release()            
#             if a != True:
#                 counter = counter + 1 
#                 print(f'[{id}][{i}]: ret = {a}, error counter = {counter}','='*10)      
#             print(f'[{id}][{i}]: Time cost = {time.time() - start}\n')
            
      
#     try:
#         if   True:
#             threading._start_new_thread( version_test, (1000000,) )
#             threading._start_new_thread( record_test, (5000000, 0.1, rec_dev[0], wavefile_record, 16000, 16, 2, ) )     
#             threading._start_new_thread( play_test, (1000000, 5, play_dev[1], wavefile_play) )
#             threading._start_new_thread( play_test, (1000000, 3, play_dev[2], wavefile_play) )
#             threading._start_new_thread( play_test, (1000000, 50, play_dev[0], wavefile_play) )

#         else:
#             threading._start_new_thread( play_test, (1000000, 10, play_dev[3], wavefile_play) )        
#             threading._start_new_thread( record_test, (1000000, 0.2, rec_dev[2], wavefile_record_2, 16000, 16, 2, ) )
#             threads = threading.Thread( target = version_test, args = (100,) )
#             thread.setDaemon(True)
#             threads.start()
#             threads = threading.Thread( target = version_test, args = (200,) )
#             thread.setDaemon(True)
#             threads.start()
#             threads = threading.Thread( target = version_test, args = (500,) )
#             thread.setDaemon(True)
#             threads.start()
        
#     except Exception as e:
#         print(f"Error: unable to start thread:  {e}") 
    
#     while True:
#         cnt= threading.activeCount() 
#         print(f"\n ----------- threading.activeCount = {cnt} -----------\n")
#         if cnt > 0 :
#             sleep(2)
#         else:
#             pass
#     print("Test finished!")


def serial_recv(serial):
    while True:
        data = serial.read_all()
        if data == '':
            continue
        else:
            break

        time.sleep(0.1)
    return data


if __name__ == '__main__':
    eraseDone = 0
    
    parser = argparse.ArgumentParser(description='Virtual Serial Port Cable(VSPC) is a tool used to realize serial port to serial port data transfer')
    parser.add_argument('port_a', type=str, help="Serial port A('/dev/tty.usbxxxxx' for UNIX-like systems or 'COMx' for Windows")
    parser.add_argument('port_b', type=str, help="Serial port B('/dev/tty.usbxxxxx' for UNIX-like systems or 'COMx' for Windows")
    parser.add_argument('baudrate', type=int, default= 115200, help="Serial port baud rate, default=115200")
 
    args = parser.parse_args()
    # print(args.echo())

    # serialA = CSerial(com_portA, baud_rate)
    # serialB = CSerial(com_portB, baud_rate)
    
    try:
        serialA = serial.Serial(args.port_a, baudrate=args.baudrate, timeout=0.01) # serial.Serial('COM5', 115200, timeout=0.5)  
        if not serialA.isOpen() :
            print(f"open {args.port_a} failed")
            assert(False)
        serialB = serial.Serial(args.port_b, baudrate=args.baudrate, timeout=0.01)
        if not serialB.isOpen() :
            print(f"open {args.port_b} failed")
            assert(False)    
    except Exception as e:
        print(f"Error:  {e}")
        exit(0) 

    while True:
        dataA =serial_recv(serialA)
        if dataA != b'' :
            #print(f"{args.port_a} receive : {len(data)}B",end="")            
            serialB.write(dataA) #数据写回
        dataB = b''
        # dataB =serial_recv(serialA)
        # if dataB != b'' :
        #     #print(f"{args.port_b} receive : {len(data)}B",end="\r")
        #     #print("%s receive: %5d B" % (args.port_b, len(data)), end='\r')
        #     serialB.write(dataB) #数据写回
        if dataA != b'' or dataB != b'' :
            print("%s receive: %5d B,  %s receive: %5d B" % (args.port_a, len(dataA), args.port_b, len(dataB)), end='\n')

              


 

