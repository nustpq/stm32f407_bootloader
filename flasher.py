from __future__ import print_function
import sys, serial, binascii, time, struct, argparse
from intelhex import IntelHex

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
class ProgramModeError(Exception):
    pass

class TimeoutError(Exception):
    pass

class STM32Flasher(object):
    
    def __init__(self, serialPort, baudrate=115200):
        self.serial = None
        try:
            self.serial = serial.Serial(serialPort, baudrate=baudrate, timeout=2)
            self.update_cmd = list_cmd_update_v1            
        except :
            print(f"Error: Failed to open {serialPort}.")            
    def _sstr_(self,data):       	
        self.serial.write(data)       
    
    def check_mcu_version(self):        
        print("Check MCU FW Version...")
        self.serial.write(self._create_cmd_message(list_cmd_standby))
        time.sleep(0.1)
        self.serial.flushInput()
        self.serial.write(self._create_cmd_message(list_cmd_version))
        ret = self.serial.read(20)      
        if len(ret) > 0:  
            #print(ret)
            try:       
                ret_str = str(ret, encoding = "utf-8")            
                print(f"Current Version: {ret_str}")
                if '[FW:V' in ret_str :
                    self.update_cmd = list_cmd_update_v2
                    return True
                else:
                    print("Error: Firmware version info format error.")#format error, '[FW:Vx.x]' should be included in version string.
                    return False
            except :
                print("Old version firmware(V2.x) detected.")#old version Master not support cmd_version, return voice data
                return False
        else:           
            print("Old version firmware(V2.x) detected.")#old version Slave not support cmd_version, no data return
            return False

    def reboot_mcu(self):        
        print("Rebooting MCU ...")      
        self.serial.write(self._create_cmd_message(self.update_cmd))
        time.sleep(0.2)
        self.serial.flushInput()
        ret = self.serial.read(100)        
        if len(ret) > 0:              
            try:
                ret_str = str(ret, encoding = "utf-8")
                if 'Bootloader' in ret_str: 
                    return True
                else:               
                    print("Reboot MCU aborted.")
                    return False
            except :
                print("Parse bootload info failed.")
                return False
        else:
           # raise TimeoutError("Timeout error")
            print("Reboot MCU abort")
            return False
      
    def _crc_stm32(self, data):
        #Computes CRC checksum using CRC-32 polynomial 
        crc = 0xFFFFFFFF

        for d in data:
            crc ^= d
            for i in range(32):
                if crc & 0x80000000:
                    crc = (crc << 1) ^ 0x04C11DB7 #Polynomial used in STM32
                else:
                    crc = (crc << 1)

        return (crc & 0xFFFFFFFF)

    def _create_cmd_message(self, msg):
        #Encodes a command adding the CRC32
        cmd = list(msg) + list(struct.pack("I", self._crc_stm32(msg)))
        return cmd    
	
    def _write_protection(self, sector):
        self.serial.flushInput()
        data = self.serial.read(1)
        self.serial.write(self._create_cmd_message((CMD_WP_ON,sector)))
        if len(data) == 1:
            if struct.unpack("b", data)[0] != ACK:
                #raise ProgramModeError("Can't remove write protection")
                print("Error: No ACK") 
                return False 
            else:
                return True           
        else:
            #raise TimeoutError("Timeout error")
            return False

    def _Re_write_protection(self, sector):
        self.serial.flushInput()
        self.serial.write(self._create_cmd_message((CMD_WP_OFF,sector)))
        data = self.serial.read(1)
        if len(data) == 1:
            if struct.unpack("b", data)[0] != ACK:
                #raise ProgramModeError("Can't remove write protection")
                print("Error: No ACK") 
                return False 
            else:
                return True           
        else:
            #raise TimeoutError("Timeout error")
            return False
	
    def _jump(self, address):
        print("Start run new App...")
        self.serial.flushInput()
        #self.serial.write(self._create_cmd_message(([CMD_JUMP] +map(ord, struct.pack("I", address)))))
        self.serial.write(self._create_cmd_message([CMD_JUMP] + list( struct.pack("I", address))))
        # data = self.serial.read(1)
        # print(data)
        # if len(data) == 1:
        #     if struct.unpack("b", data)[0] != ACK:
        #         raise ProgramModeError(f"Can't jump.({data})")
        # else:
        #     raise TimeoutError("Timeout error")

    def _update(self, address):
        print(f"Updating app start address...")
        self.serial.flushInput()
        #self.serial.write(self._create_cmd_message(([CMD_UPDATE] +map(ord, struct.pack("I", address)))))
        self.serial.write(self._create_cmd_message([CMD_UPDATE] + list( struct.pack("I", address))))
        data = self.serial.read(1)
        if len(data) == 1:
            if struct.unpack("b", data)[0] != ACK:
                #raise ProgramModeError(f"Can't update.({data})")
                print("Error: No ACK") 
                return False 
            else:
                return True           
        else:
            #raise TimeoutError("Timeout error")
            return False

    def eraseFLASH(self, nsectors):
        #Sends an CMD_ERASE to the bootloader
        print(f"Erasing sector {nsectors}...")
        if nsectors > 11:
            #raise TimeoutError("sector invalid!")
            print("Error: sector invalid!") 
            return False
        if nsectors == 0: #sector 0 is bootloader, must be reserved.
            #raise TimeoutError("sector 0 is reserved for bootloader!") 
            print("Error: Sector 0 is reserved for bootloader!") 
            return False
        self.serial.flushInput()
        self.serial.write(self._create_cmd_message((CMD_ERASE,nsectors<<3)))
        data = self.serial.read(1)
        if len(data) == 1: 
            if struct.unpack("b", data)[0] != ACK:
                #raise ProgramModeError(f"Can't erase FLASH.({data})")
                print("Error: No ACK") 
                return False 
            else:
                return True           
        else:
            #raise TimeoutError("Timeout error")
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
        while addr <= ih.maxaddr():
            if not resend:
                data = []
                saddr = addr
                for i in range(256):
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
                     print("\nError: resend >= 3")
                     break

                self.serial.flushInput()        
                self.serial.write(self._create_cmd_message([CMD_WRITE] + list( struct.pack("I", saddr))))
                ret = self.serial.read(1)
                if len(ret) == 1:
                    if struct.unpack("b", ret)[0] != ACK:
                        raise ProgramModeError(f"Write abort.({ret})")
                else:
                    raise TimeoutError("Timeout error")
                encdata =data # self._encryptMessage(data)
                self.serial.flushInput()
                self.serial.write(self._create_cmd_message(data))
                ret = self.serial.read(1)
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
    eraseDone = 0   
    parser = argparse.ArgumentParser(description="Loads a Intel HEX file to update STM32F407 App firmare by bootloader")
    parser.add_argument('com_port', metavar='com_port_path', type=str, help="Serial port ('COMx' for Windows or '/dev/tty.usbxxxxx' for Linux ")
    parser.add_argument('hex_file', metavar='hex_file_path', type=str, help="Path to the Intel HEX file containing the firmware to update")
    args = parser.parse_args()

    def doErase(arg):
        global eraseDone        
    
    flasher = STM32Flasher(args.com_port)
    if flasher.serial==None:
        print("\nError: Abort firmware updating.") 
        sys.exit()  	
    flasher.check_mcu_version()
    print('+'*64)
    print("+ To avoid permanent damage, do NOT power off during updating! +")
    print('+'*64)
    input(f"\nPress 'Enter' key to start downloading [{args.hex_file}] via [{args.com_port}] \n")
    start_time = time.time()  
    flasher.reboot_mcu()  #note: remove if no app running
    flasher._sstr_("s".encode())   
 
    print("\nErasing app flash sectors...")
    sect_list = [1,2,3,4,5,6,7]
    for i in range(len(sect_list)) : 
        ret = flasher.eraseFLASH(sect_list[i])  
        if not ret:            
            print("\nError: Abort firmware updating.") 
            if i>0:
                print("Error: App might have been damaged, need return to factory!") 
            sys.exit()  
     
    print(f"\nLoading <{args.hex_file}> hex file... ")   
        
    for e in flasher.writeImage(sys.argv[2]):
        if "saddr" in e:
            print("Start address : 0x%X" %e["saddr"])
            print("End address   : 0x%X\n" %e["eaddr"])
        if "loc" in e:
            if e["resend"] > 0:
                end = f'\nReSend={e["resend"]}\n'
            else:
                end = ""
            _time = time.time() - start_time      
            percentage = int(round(  (e["loc"]-start_address)*100/(end_address-start_address), 1))
            print("\rWriting address [0x%X]...  %ds.\t\t[%d%% Done]" % (e["loc"],round(_time,0), percentage), end=end)
            sys.stdout.flush()

        if "success" in e and e["success"]:                    
            print("\nUpdate start address: ", hex(start_address))
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
                print("\nError: Update operation failed!")
                sys.exit() 
            print("\nDone")

        elif "success" in e and not e["success"]:
            print("\nError: Failed to download firmware!") 
            break        
            
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
    print("Firmware update seccessfully!")				
			
        
	
