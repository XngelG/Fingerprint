from pyModbusTCP.client import ModbusClient
import os

with open('ipconfig.txt') as f:
    lines = f.readlines()
ownIp = lines[0].strip()
os.system('sudo ifconfig eth0 down')
os.system('sudo ifconfig eth0 '+ownIp)
os.system('sudo ifconfig eth0 up')

plcIp = lines[1].strip()
f.close()
c = ModbusClient(host=plcIp, port=502, unit_id=1, auto_open=True)

class Registro():
    def __init__(self, ID, Nombre):
        self.ID = ID
        self.Nombre = Nombre

registro = []        
with open('registerArray.txt') as g:
    lines = g.readlines()
for i in lines:
    registro.append(Registro(int(i.split(',')[0]),i.split(',')[1].strip()))
g.close()

ReadReq = False
ReadCmpltClr = False
DeleteReq = False
DeleteCmpltClr = False
RegisterReq = False
RegCmpltClr = False
ReadReqAckClr = False
RegAckClr = False

ReadReqAck = False
ReadyStatus = False
ReadComplete = False
DeleteAck = False
DeleteComplete = False
RegisterAck = False
RegisterComplete = False

def ReadBits():
    try:
        read = c.read_coils(16,8)
        return read[0],read[1],read[2],read[3],read[4],read[5],read[6],read[7]
    except:
        return 0,0,0,0,0,0,0,0

def WriteSingleCoil(num,val):
    try:
        c.write_single_coil(num,val)
    except:
        pass

ReadyStatus = True
try:
    c.write_single_coil(1,ReadyStatus)
except:
    pass
while True:
    ReadReq, ReadCmpltClr, DeleteReq, DeleteCmpltClr, RegisterReq, RegCmpltClr, ReadReqAckClr, RegAckClr = ReadBits()

    if ReadReq:
        ReadyStatus = False
        WriteSingleCoil(1,ReadyStatus)
        ReadReqAck = True
        WriteSingleCoil(0,ReadReqAck)
        ##proceso de leer huella
        FingerprintResult = 1
        FingerprintName = []
        if FingerprintResult != -1:
            for i in registro:
                if i.ID == FingerprintResult:
                    for j in i.Nombre:
                        FingerprintName.append(ord(j))
                    break
        try:
            c.write_multiple_registers(4,FingerprintName)
            c.write_single_register(0,FingerprintResult)
        except:
            pass           
        ReadComplete = True
        WriteSingleCoil(2,ReadComplete)
        ReadyStatus = True
        WriteSingleCoil(1,ReadyStatus)
        
    if ReadReqAckClr:
        ReadReqAck = False
        WriteSingleCoil(0,ReadReqAck)
        
    if ReadCmpltClr:
        ReadComplete = False
        WriteSingleCoil(2,ReadComplete)
        
    if RegisterReq:
        ReadyStatus = False
        WriteSingleCoil(1,ReadyStatus)
        RegisterAck = True
        WriteSingleCoil(5,RegisterAck)
        try:
            FingerprintNoReq = c.read_holding_registers(32,1)[0]
            FingerprintNameReq = ""
            for i in c.read_holding_registers(37,20):
                FingerprintNameReq += chr(i)
        except:
            pass
        ##proceso de registro huella

        registerSuccess = False
        for i in registro:
            if i.ID == FingerprintNoReq:
                i.Nombre = FingerprintNameReq
                registerSuccess = True
                break
        if not registerSuccess:
            registro.append(Registro(FingerprintNoReq,FingerprintNameReq))
        f = open("registerArray.txt","w")
        for i in registro:
            f.write(str(i.ID)+","+i.Nombre+"\n")
        f.close()        
        RegisterComplete = True
        WriteSingleCoil(6,RegisterComplete)
        ReadyStatus = True
        WriteSingleCoil(1,ReadyStatus)
        
    if RegAckClr:
        RegisterAck = False
        WriteSingleCoil(5,RegisterAck)
        
    if RegCmpltClr:
        RegisterComplete = False
        WriteSingleCoil(6,RegisterComplete)

    if DeleteReq:
        ReadyStatus = False
        WriteSingleCoil(1,ReadyStatus)
        try:
            DeleteNoReq = c.read_holding_registers(33,1)[0]
        except:
            pass
        #eliminar huella
        for i in range(len(registro)):
            if registro[i].ID == DeleteNoReq:
                registro.pop(i)
                break
        f = open("registerArray.txt","w")
        for i in registro:
            f.write(str(i.ID)+","+i.Nombre+"\n")
        f.close()
        DeleteComplete = True
        WriteSingleCoil(4,DeleteComplete)
        ReadyStatus = True
        WriteSingleCoil(1,ReadyStatus)
        
    if DeleteCmpltClr:
        DeleteComplete = False
        WriteSingleCoil(4,DeleteComplete)

        
