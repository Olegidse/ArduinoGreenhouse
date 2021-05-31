void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data ) 
{
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
 
  delay(5);
}
 
byte readEEPROM(int deviceaddress, unsigned int eeaddress ) 
{
  byte rdata = 0xFF;
 
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(deviceaddress,1);
 
  if (Wire.available()) rdata = Wire.read();

  return rdata;
}


// This function will write a 4 byte (32bit) long to the eeprom at
// the specified address to address + 3.
void EEPROMWritelong(int address, long value) {
      
      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

      //Write the 4 bytes into the eeprom memory.
      writeEEPROM(disk0, address, four);
      writeEEPROM(disk0, address + 1, three);
      writeEEPROM(disk0, address + 2, two);
      writeEEPROM(disk0, address + 3, one);
}

// This function will return a 4 byte (32bit) long from the eeprom
// at the specified address to address + 3.
long EEPROMReadlong(int address)
      {
      //Read the 4 bytes from the eeprom memory.
      long four = readEEPROM(disk0, address);
      long three = readEEPROM(disk0, address + 1);
      long two = readEEPROM(disk0, address + 2);
      long one = readEEPROM(disk0, address + 3);

      //Return the recomposed long by using bitshift.
      return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
