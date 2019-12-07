import socket
sc=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
adres=('127.0.0.1',102)
sc.connect(adres)
#connection
mesaj1=b'\x03\x00\x00\x16\x11\xE0\x00\x00\x00\x01\x00\xC0\x01\x0A\xC1\x02\x01\x00\xC2\x02\x01\x02'
sc.send(mesaj1)
data = sc.recv(50)
print(len(data))
#pdu negotiation
mesaj2=b'\x03\x00\x00\x19\x02\xf0\x80\x32\x01\x00\x00\x04\x00\x00\x08\x00\x00\xf0\x00\x00\x01\x00\x01\x00\xf0'
sc.send(mesaj2)
data = sc.recv(50)
print(len(data))
print("PDU lenght",data[-1])
#22
#27
#240
sc.close()
