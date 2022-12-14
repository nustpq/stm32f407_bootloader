from __future__ import print_function
import sys, serial, binascii, time, struct, argparse, math, zlib
from intelhex import IntelHex
from tqdm import tqdm, trange

ACK  = 0x55
NACK = 0xAA
CMD_GETID  = 0x90
CMD_ERASE  = 0xA1
CMD_WRITE  = 0xB2
CMD_UPDATE = 0xC3
CMD_JUMP   = 0xD4
CMD_WP_ON  = 0xE5
CMD_WP_OFF = 0xF6

list_cmd_version   = [0xEB, 0x90, 0x00 ,0x07 ,0x00 ,0x07 ,0xF0]
list_cmd_standby   = [0xEB, 0x90, 0x00 ,0x08 ,0x22 ,0x00 ,0x2A ,0xF0]
list_cmd_update_v2 = [0xEB, 0x90, 0x00 ,0x08 ,0x77 ,0x77 ,0x08 ,0xF0]
list_cmd_update_v1 = [0xEB ,0x00 ,0x07 ,0x77 ,0x77 ,0x07 ,0xF0]

# class bcolors:
#     HEADER = '\033[95m'
#     OKBLUE = '\033[94m'
#     OKGREEN = '\033[92m'
#     WARNING = '\033[5;30;43m'
#     ERROR = '\033[5;37;41m'
#     FAIL = '\033[91m'
#     ENDC = '\033[0m'
#     BOLD = '\033[1m'
#     BOLD2 = '\033[1;34;40m'
#     UNDERLINE = '\033[4m'

bcolors = {
    'HEADER': '\033[95m',
    'ENDC': '\033[0m',
    
    'OKBLUE': '\033[94m',
    'OKGREEN': '\033[92m',
    'INFO': '\033[5;30;42m',
    'WARN': '\033[5;30;43m',
    'ERROR': '\033[5;37;41m',
    'FAIL': '\033[91m',
    'BOLD': '\033[1m',
    'BOLD2': '\033[1;34;40m',
    'UNDERLINE': '\033[4m',
}
def colorstr(info, color):
    if color not in bcolors:
        return info
    return bcolors[color]+str(info)+bcolors['ENDC']

def printc(info, type):
    if type=='ERROR':
        print(colorstr('Error','ERROR')+': '+str(info))
    elif type=='WARN':
        print(colorstr('Warning','WARN')+': '+str(info))
    elif type=='INFO':
        print(colorstr('Info','INFO')+': '+str(info) )
    else:
        print(str(info))
        
class ProgramModeError(Exception):
    pass

class TimeoutError(Exception):
    pass

#crc_time = 0 
def crc_stm32(data_list):
    #Computes CRC checksum using CRC-32 polynomial 
    # global crc_time
    # t0=time.time()
    crc = 0xFFFFFFFF
    for v in data_list:
        crc ^= v
        for i in range(32):
            if crc & 0x80000000:
                crc = (crc << 1) ^ 0x04c11db7 #Polynomial used in STM32
            else:
                crc = (crc << 1)
    # crc_time+=time.time()-t0
    return (crc & 0xFFFFFFFF)

class STM32Flasher(object):
    
    def __init__(self, serialPort, baudrate=115200):
        self.serial = None
        try:
            self.serial = serial.Serial(serialPort, baudrate=baudrate, timeout=2)
            self.update_cmd = list_cmd_update_v1            
        except :
            printc(f"Failed to open {serialPort}.",'ERROR')            
    def _sstr_(self,data):       	
        self.serial.write(data)  
    
    def send_cmder(self, cmd_list, byte_num=0, delay_s=0): 
        ret = b''
        self.serial.flushInput()
        self.serial.write(self._create_cmd_message(cmd_list))
        if delay_s>0:
            time.sleep(delay_s)
        self.serial.flushInput()
        if byte_num >0 :
            ret = self.serial.read(byte_num)      
        return ret
    
    def check_mcu_version(self):        
        print("Check MCU firmware version...")
        self.send_cmder(list_cmd_standby)
        time.sleep(0.1)        
        ret = self.send_cmder(list_cmd_version,20)   
        if len(ret) > 0:  
            #print(ret)
            try:       
                ret_str = str(ret, encoding = "utf-8")            
                print(colorstr(f"{ret_str}","OKGREEN"))
                if '[FW:V' in ret_str :
                    self.update_cmd = list_cmd_update_v2
                    return True
                else:
                    printc("Firmware version info format error.",'ERROR')#format error, '[FW:Vx.x]' should be included in version string.
                    return False
            except :
                printc("Old version firmware(V2.x) detected.",'WARN')#old version Master not support cmd_version, return voice data
                return False
        else:           
            printc("Old version firmware(V2.x) detected.",'WARN')#old version Slave not support cmd_version, no data return
            return False

    def reboot_mcu(self):        
        print("Rebooting MCU ...")      
        ret = self.send_cmder(self.update_cmd, 100, 0.2)      
        if len(ret) > 0:              
            try:
                ret_str = str(ret, encoding = "utf-8")
                if 'Bootloader' in ret_str: 
                    return True
                else:               
                    printc("Reboot MCU aborted.",'WARN')
                    return False
            except :
                printc("Parse bootload info failed.",'WARN')
                return False
        else:
           # raise TimeoutError("Timeout error")
            printc("Reboot MCU aborted.",'WARN')
            return False

    def _create_cmd_message(self, msg):
        #Encodes a command adding the CRC32
        cmd = list(msg) + list(struct.pack("I", crc_stm32(msg)))
        return cmd    
	
    def _write_protection(self, sector):
        data = self.send_cmder((CMD_WP_ON,sector),1)
        if len(data) == 1:
            if struct.unpack("b", data)[0] != ACK:      
                printc("No ACK",'ERROR')
                return False 
            else:
                return True           
        else:         
            return False

    def _Re_write_protection(self, sector):
        data = self.send_cmder((CMD_WP_OFF,sector),1)
        if len(data) == 1:
            if struct.unpack("b", data)[0] != ACK:       
                printc("No ACK",'ERROR')
                return False 
            else:
                return True           
        else:    
            return False
	
    def _jump(self, address):
        print("Start run new App...")
        self.send_cmder([CMD_JUMP] + list( struct.pack("I", address)))  

    def _update(self, address):
        print(f"Updating app start address... [{hex(start_address)}]") 
        data = self.send_cmder([CMD_UPDATE] + list( struct.pack("I", address)),1) 
        if len(data) == 1:
            if struct.unpack("b", data)[0] != ACK:         
                printc("No ACK",'ERROR')
                return False 
            else:
                return True           
        else: 
            return False

    def eraseFLASH(self, nsectors):
        #Sends an CMD_ERASE to the bootloader
        print(f"Erasing sector {nsectors}...")
        if nsectors > 11:      
            printc("Sector invalid!",'ERROR') 
            return False
        if nsectors == 0: #sector 0 is bootloader, must be reserved.      
            printc("Sector 0 is reserved for bootloader!",'ERROR')
            return False
        data = self.send_cmder((CMD_ERASE,nsectors<<3),1) 
        if len(data) == 1: 
            if struct.unpack("b", data)[0] != ACK:          
                printc("No ACK",'ERROR')
                return False 
            else:
                return True           
        else:
            return False

    def writeImage(self, filename):
        #Sends an CMD_WRITE to the bootloader
        #This is method is a generator, that returns its progresses to the caller.
        #In this way, it's possible for the caller to live-print messages about
        #writing progress 
        ih = IntelHex()  
        ih.loadhex(filename)
        yield {"saddr": ih.minaddr(), "eaddr": ih.maxaddr()}
        global start_address
        addr = ih.minaddr()
        start_address = addr
        global end_address
        end_address = ih.maxaddr()
        content = ih.todict()
        abort = False
        resend = 0
        pack_size = 256
        #while addr <= ih.maxaddr():
        for i in trange(math.ceil((end_address-start_address)/pack_size),desc ='Downloading',unit='p',dynamic_ncols=True, colour = 'green'):
            if not resend:
                data = []
                saddr = addr
                for i in range(pack_size):
                  try:
                      data.append(content[addr])
                  except KeyError:
                      #if the HEX file doesn't contain a value for the given address
                      #we "pad" it with 0xFF, which corresponds to the erase value
                      data.append(0xFF)
                  addr+=1
            try:
                if resend >= 3:
                     abort = True
                     printc("Resend >= 3.",'ERROR')
                     break   
                ret = self.send_cmder([CMD_WRITE] + list( struct.pack("I", saddr)), 1) 
                if len(ret) == 1:
                    if struct.unpack("b", ret)[0] != ACK:
                        raise ProgramModeError(f"Write abort.({ret})")
                else:
                    raise TimeoutError("Timeout error")
                encdata =data # self._encryptMessage(data)      
                ret = self.send_cmder(data, 1) 
                if len(ret) == 1:
                    if struct.unpack("b", ret)[0] != ACK:
                        raise ProgramModeError("Write abort")
                else:
                    raise TimeoutError("Timeout error")

                yield {"loc": saddr, "resend": resend}
                resend = 0
            except (TimeoutError, ProgramModeError):
                resend +=1         

        yield {"success": not abort}


        
        
if __name__ == '__main__':
    print('+'*64)
    print('+'+' '*21+ colorstr('Firmware Updater V3','BOLD2') +' '*22+'+')
    print('+ To avoid permanent damage, '+colorstr('DO NOT ','FAIL')+'power off during updating! +')    
    print('+'*64)    
    eraseDone = 0  
    parser = argparse.ArgumentParser(description="Loads hex file to update STM32F407 App firmware via COM port by easy bootloader inside")
    parser.add_argument('com_port', metavar='com_port_path', type=str, help="Serial port ('COMx' for Windows or '/dev/tty.usbxxxxx' for Linux ")
    parser.add_argument('hex_file', metavar='hex_file_path', type=str, help="Path to the Intel HEX file containing the firmware to update")
    args = parser.parse_args()
    
    def doErase(arg):
        global eraseDone        
    
    flasher = STM32Flasher(args.com_port)
    if flasher.serial==None:
        printc("Abort firmware updating.",'ERROR') 
        sys.exit()  	
    flasher.check_mcu_version()
    input(f"\nPress 'Enter' key to start downloading [{args.hex_file}] via [{args.com_port}] \n")
    start_time = time.time()  
    flasher.reboot_mcu()  #note: remove if no app running
    flasher._sstr_("s".encode())   
 
    print("Erasing app flash sectors...")
    sect_list = [1,2,3,4,5,6,7]
    for i in range(len(sect_list)) : 
        ret = flasher.eraseFLASH(sect_list[i])  
        if not ret:            
            printc("Abort firmware updating.",'ERROR') 
            if i>0:
                printc("App might have been damaged, need return to factory!",'ERROR') 
            sys.exit()  
     
    print(f"\nLoading hex file <{args.hex_file}>...")   
        
    for e in flasher.writeImage(sys.argv[2]):
        if "saddr" in e:
            print(f"Address Range:[ {hex(e['saddr'])} - {hex(e['eaddr'])} ]\n")
            
        # if "loc" in e:
        #     if e["resend"] > 0:
        #         end = f'\nReSend={e["resend"]}\n'
        #     else:
        #         end = ""
        #     _time = time.time() - start_time      
        #     percentage = int(round(  (e["loc"]-start_address)*100/(end_address-start_address), 1) )
        #     print("\rWriting address [0x%X]...  %ds.\t\t[%d%% Done]" % (e["loc"],round(_time,0), percentage), end=end)
        #     sys.stdout.flush()

        if "success" in e and e["success"]:                    
            # print("\nUpdate start address: ", hex(start_address))
            # print("\n\Erase last sector ")
            # if start_address == 0x08020000:
            #     flasher.eraseFLASH(0x30)
            # if start_address == 0x08040000:
            #     flasher.eraseFLASH(0x28)		
            # time.sleep(1)
            # eraseDone = 1				
            # print("Erasing Flash memory", end="") 
            # while eraseDone == 0:		
            #     print(".", end="")
            #     sys.stdout.flush()
            #     time.sleep(0.5)
            ret = flasher._update(start_address)
            if not ret:
                printc("Update operation failed! App might have been damaged, need return to factory!",'ERROR')
                sys.exit() 

        elif "success" in e and not e["success"]:
            printc("Failed to download firmware! App might have been damaged, need return to factory!",'ERROR') 
            sys.exit()       
            
    # print("\nwant to add write protection?")
    # value = input("Please enter yes or No:\n")
    # if value == "yes":
    #     print("\n\enter sector")
    #     value = input("Please enter sector num\n")
    #     flasher._write_protection(value)
    # else:
    #     print("\n\ok")

    # print("\nwant to remove write protection?")
    # value = input("Please enter yes or No:\n")
    # if value == "yes":
    #     print("\n\enter sector")
    #     value = input("Please enter sector num\n")
    #     flasher._Re_write_protection(value)
    # else:
    #     print("\n\ok")	

    time.sleep(0.5)
    flasher._jump(start_address)
    printc("Firmware update successfully!",'INFO')	
    #print(crc_time)			
			
        
	
