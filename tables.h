const u8 dutyTable[]  asm("dutyTable") __attribute__ ((used)) = {//waveforms for channels 1 and 2.
	0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 0
};
u8 waveTable[160];//the waveforms to choose from, each is 32 bytes, ranging from 0-15. More can be added.

u8 drumTable[78];//the drum pointers

u8 drumData[200];//the drum data must be less than 256 bytes if using 8-bit pointers

const u8 waveShift[] = {//for volume control.
	4, 0, 1, 2
};
const u8 lengthLoadMask[] = {//for length-enabled notes. all channels except 3 use 6-bit.
	0x3F, 0x3F, 0xFF, 0x3F
};
const u8 NR52Mask[] = {//for disabling specific channels when length expires.
	0b01110111, 0b10111011, 0b11011101, 0b11101110
};
const u16 freqTableGB[] PROGMEM = {//starting with C3, one for each note the gameboy can handle.
	  44, 156, 262, 363, 457, 547, 631, 710, 786, 854, 923, 986,
	1046,1102,1155,1205,1253,1297,1339,1379,1417,1452,1486,1517,
	1546,1575,1602,1627,1650,1673,1694,1714,1732,1750,1767,1783,
	1798,1812,1825,1837,1849,1860,1871,1881,1890,1899,1907,1915,
	1923,1930,1936,1943,1949,1954,1959,1964,1969,1974,1978,1982,
	1985,1988,1992,1995,1998,2001,2004,2006,2009,2011,2013,2015
};

