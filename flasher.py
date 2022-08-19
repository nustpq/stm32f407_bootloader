from __future__ import print_function
import sys, serial, binascii, time, struct, argparse
from intelhex import IntelHex

ACK    = 0x55
NACK   = 0xAA
CMD_GETID  = 0x90
CMD_ERASE  = 0xA1
CMD_WRITE  = 0xB2
CMD_UPDATE = 0xC3
CMD_JUMP   = 0xD4
CMD_WP_ON  = 0xE5
CMD_WP_OFF = 0xF6

class ProgramModeError(Exception):
    pass

class TimeoutError(Exception):
    pass

class STM32Flasher(object):
    def __init__(self, serialPort, baudrate=115200):
        self.serial = serial.Serial(serialPort, baudrate=baudrate, timeout=10)
	
    def _sstr_(self,data):       	
        self.serial.write(data)
        ky = input("press any key to start\n")
      
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
        self.serial.write(self._create_cmd_message((CMD_WP_ON,sector)))
        data = self.serial.read(1)
        if len(data) == 1:
            if struct.unpack("b", data)[0] != ACK:
                raise ProgramModeError("Can't remove write protection")
        else:
            raise TimeoutError("Timeout error")

    def _Re_write_protection(self, sector):
        self.serial.flushInput()
        self.serial.write(self._create_cmd_message((CMD_WP_OFF,sector)))
        data = self.serial.read(1)
        if len(data) == 1:
            if struct.unpack("b", data)[0] != ACK:
                raise ProgramModeError("Can't remove write protection")
        else:
            raise TimeoutError("Timeout error")
	
    def _jump(self, address):
        self.serial.flushInput()
        #self.serial.write(self._create_cmd_message(([CMD_JUMP] +map(ord, struct.pack("I", address)))))
        self.serial.write(self._create_cmd_message([CMD_JUMP] + list( struct.pack("I", address))))
        data = self.serial.read(1)
        if len(data) == 1:
            if struct.unpack("b", data)[0] != ACK:
                raise ProgramModeError(f"Can't jump.({data})")
        else:
            raise TimeoutError("Timeout error")

    def _update(self, address):
        self.serial.flushInput()
        #self.serial.write(self._create_cmd_message(([CMD_UPDATE] +map(ord, struct.pack("I", address)))))
        self.serial.write(self._create_cmd_message([CMD_UPDATE] + list( struct.pack("I", address))))
        data = self.serial.read(1)
        if len(data) == 1:
            if struct.unpack("b", data)[0] != ACK:
                raise ProgramModeError(f"Can't update.({data})")
        else:
            raise TimeoutError("Timeout error")

    def eraseFLASH(self, nsectors):
        #Sends an CMD_ERASE to the bootloader
        print(f"erasing sector[{nsectors}]...")
        if nsectors > 11:
            raise TimeoutError("sector invalid!") 
        if nsectors == 0:
            raise TimeoutError("sector 0 is reserved for bootloader!") 
        self.serial.flushInput()
        self.serial.write(self._create_cmd_message((CMD_ERASE,nsectors<<3)))
        data = self.serial.read(1)
        if len(data) == 1:
            if struct.unpack("b", data)[0] != ACK:
                raise ProgramModeError(f"Can't erase FLASH.({data})")
        else:
            raise TimeoutError("Timeout error") 

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
        content = ih.todict()
        abort = False
        resend = 0
        while addr <= ih.maxaddr():
            if not resend:
                data = []
                saddr = addr
                for i in range(16):
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
                encdata =data# self._encryptMessage(data)
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
    
    parser = argparse.ArgumentParser(description='Loads a IntelHEX binary file using the custom bootloader described in the "MasteringSTM32 book')
    parser.add_argument('com_port', metavar='com_port_path', type=str, help="Serial port ('/dev/tty.usbxxxxx' for UNIX-like systems or 'COMx' for Windows")
    parser.add_argument('hex_file', metavar='hex_file_path', type=str, help="Path to the IntelHEX file containing the firmware to flash on the target MCU")
    args = parser.parse_args()

    def doErase(arg):
        global eraseDone        
    
    flasher = STM32Flasher(args.com_port)	
    flasher._sstr_("s".encode())
   
    print("\nwant to upgrade the firmware?")
    value = input("Please enter yes or No:\n")

    if value == "yes":
        print("\nErase app flash ...")
        sect_list = [1,2,3,4,5,6,7]
        for i in range(len(sect_list)) : #sector 0 is bootloader,need be reserved.
            flasher.eraseFLASH(sect_list[i])    

    start_time = time.time()   
    print(f"Loading HEX file... {args.hex_file}")        
    for e in flasher.writeImage(sys.argv[2]):
        if "saddr" in e:
            print("Start address: 0x%X" %e["saddr"])
            print("End address  : 0x%X\n" %e["eaddr"])
        if "loc" in e:
            if e["resend"] > 0:
                end = f'\nReSend={e["resend"]}\n'
            else:
                end = ""
            _time = time.time() - start_time
            print("\rWriting address[0x%X]...  %ds" % (e["loc"],round(_time,0)), end=end)
            sys.stdout.flush()

        if "success" in e and e["success"]:
            print("Done\n")             
            print("Update start address: ", hex(start_address))
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
            flasher._update(start_address)
            print("Done\n")

        elif "success" in e and not e["success"]:
            print("\nFailed to upload firmware")         
            
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

    print("Start App...")
    time.sleep(0.5)
    flasher._jump(start_address)				
			
        
	
