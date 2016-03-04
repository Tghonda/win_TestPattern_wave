// win_TestPatternECG.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <math.h>

#include <fstream>
#include <iostream>


const static int	SamplingRate = 48000;
const static double SamplingRateF = 48000.0;

const static double dataRage = 1000.0 / 450.0;
const static int serialno = 151173;

static double currentTime = 0.0;
__int16 *pcm;
int	pcmIdx = 0;
double lastPhase = 0.0;

int _tmain(int argc, _TCHAR* argv[])
{
	static int pcmSamples = SamplingRate*60;
	pcm = (__int16 *)calloc(pcmSamples, sizeof(__int16));
	void genPattern1( void );
	void saveWave(__int16 *pcm, int samples);

	genPattern1();
	saveWave(pcm, pcmIdx);
	return 0;
}

/************************************************************************************/

typedef struct {
	union {
		char	chankId[4];
		__int32 chankIdVal;
	};
	__int32	chankSize;
} _chankHeader;

#define CHANK_RIFF	0x46464952		//  'RIFF'
#define CHANK_WAVE	0x45564157		//  'WAVE'
#define CHANK_fmt	0x20746d66		//	'fmt '
#define CHANK_data	0x61746164		//	'data'


typedef struct {
	__int32  fmtChankSize;		// 16
	__int16	 wFormatTag;		// LinearPCM:	1
	__int16	 wChannels;			// Monoral:		1
	__int32	 dwSamplesPerSec;	// 44100
	__int32	 dwAvgBytesPerSec;	// 44100*2
	__int16 wBlockAlign;		// 2
	__int16 wBitsPerSample;		// 16
} _fmtChunk;


void saveWave(__int16 *pcm, int samples)
{
	std::ofstream fs;

	fs.open("Pattern.wav", std::ios::out | std::ios::binary);
	if (fs.fail()) {
		std::cerr << "Error! cannot create output file:" << "Pattern.wav" << "\n";
		return;
	}

	_chankHeader hdr;
	_fmtChunk fmt;
	_chankHeader data;
	_int32 tagWAVE = CHANK_WAVE;
	_int32 tagfmt = CHANK_fmt;

	hdr.chankIdVal = CHANK_RIFF;
	hdr.chankSize  = sizeof(tagWAVE) + sizeof(tagfmt) + sizeof(fmt) + sizeof(data) 
		+ samples*sizeof(__int16);
	fs.write((const char*)&hdr, sizeof(hdr));

	fs.write((const char*)&tagWAVE, sizeof(tagWAVE));
	fs.write((const char*)&tagfmt, sizeof(tagfmt));

	fmt.fmtChankSize = 16;
	fmt.wFormatTag = 1;
	fmt.wChannels = 1;
	fmt.dwSamplesPerSec = SamplingRate;
	fmt.dwAvgBytesPerSec = SamplingRate *2;
	fmt.wBlockAlign = 2;
	fmt.wBitsPerSample = 16;
	fs.write((const char*)&fmt, sizeof(fmt));

	data.chankIdVal = CHANK_data;
	data.chankSize = samples*sizeof(__int16);
	fs.write((const char*)&data, sizeof(data));

	fs.write((const char*)pcm, samples*sizeof(__int16));

	fs.close();
}

void genPCM(double freq, double duration)
{
    currentTime += duration/1000.0;		// duration (msec)
    int currentPosi = (int)(SamplingRate * currentTime);

    while (pcmIdx < currentPosi) {
        pcm[pcmIdx++] = (__int16)((sin(lastPhase) * 0.8) * SHRT_MAX);
		lastPhase += 2.0*M_PI * freq / SamplingRateF;
    }
}

void genPCMT(double freq, double duration)
{
	genPCM(freq, duration);
#if 0
	int len = duration/dataRage;
	int f = (int)freq;
	while (len-- > 0)
		std::cout << f << "\n"; 
#endif
}
void genHedders( void )
{
    int i;
    double freq, f;
    
    genPCM(0.0, 2000.0);

    // Lead
    genPCM(0.0, 260.0);
    genPCM(1200.0, 40.0);
    
    //  Header
    double t = dataRage;
    for (f=0.0; f<510.0; f += t) {
        freq = f*1000.0 / 510.0 + 1200.0;
        genPCM(freq, t);
    }
    genPCM(2240.0, 28.0);
    
    // Calibration
    for (i=0; i<18; i++) {
        genPCM(1800.0, 40.0);
        genPCM(1700.0, 40.0);
        genPCM(1600.0, 40.0);
    }
    
    // Serial No
    for (i=0; i<24; i++) {
        freq = 1700.0 + ((serialno & (1<<i))? 335.0 : -335.0);
        genPCM(freq, 80.0);
    }
    int chksum = ((serialno>>16) & 0xFF) + ((serialno>>8) & 0xFF) + (serialno & 0xFF);
    for (i=0; i<16; i++) {
        freq = 1700.0 + ((chksum & (1<<i))? 335.0 : -335.0);
        genPCM(freq, 80.0);
    }
}

void pattern1( void )
{
	int i;
	double freq;

	// Data
    for (i=0; i<=1000; i+=2) {
        genPCMT(1200+i, dataRage);	// スイープアップ
    }
    for (i=1000; i>0; i-=2) {
        genPCMT(1200+i, dataRage);	// スイープダウン
    }

	// サイン波＋減衰    
    for (i=0; i<2300; i++) {
        freq = 1700.0 + sin(2.0*M_PI*8*i/2300.0) * 500.0*(1.0 - i/2300.0);
        genPCMT(freq, dataRage);
    }

	// UP/Down 100%
    for (i=0; i<50; i++) {
        genPCMT(1700-500, dataRage *2);
        genPCMT(1700+500, dataRage *2);
    }
	// UP/Down 80%
    for (i=0; i<50; i++) {
        genPCMT(1700-400, dataRage *2);
        genPCMT(1700+400, dataRage *2);
    }
	// UP/Down 60%
	for (i=0; i<50; i++) {
        genPCMT(1700-300, dataRage *2);
        genPCMT(1700+300, dataRage *2);
    }
	// UP/Down 40%
    for (i=0; i<50; i++) {
        genPCMT(1700-200, dataRage *2);
        genPCMT(1700+200, dataRage *2);
    }
	// UP/Down 20%
    for (i=0; i<50; i++) {
        genPCMT(1700-100, dataRage *2);
        genPCMT(1700+100, dataRage *2);
    }
	// UP/Down 0%
    for (i=0; i<50; i++) {
        genPCMT(1700, dataRage);
    }
    
    for (i=0; i<=10; i++) {
        genPCMT(1200+i*100, dataRage *20);
    }
    for (i=0; i<=10; i++) {
        genPCMT(1200+i*100, dataRage *20);
    }

    
	for (i=1300; i<=2100 ; i+=100) {
		for (int j=0; j<40; j++) {
			genPCMT(i+100, dataRage *2);
			genPCMT(i-100, dataRage *2);
		}
	}

	genPCMT(1400, dataRage *150);
	genPCMT(1600, dataRage *150);
	genPCMT(1800, dataRage *150);
	genPCMT(2000, dataRage *150);
	genPCMT(1700, dataRage *400);

}


void genPattern1( void )
{
    genHedders();
	pattern1();
}
