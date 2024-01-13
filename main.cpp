#include <stdio.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <winbgim.h>

#define DEBUG 1
#define N 512
#define K 256

typedef struct Header {
	char chunk_id[4];
	int chunk_size;
	char format[4];
	char subchunk1_id[4];
	int subchunk1_size;
	short int audio_format;
	short int num_channels;
	int sample_rate;
	int byte_rate;
	short int block_align;
	short int bits_per_sample;
	short int extra_param_size;
	char subchunk2_id[4];
	int subchunk2_size;
} Header;

void delay(int ms);
void init_ui();
void read_data_from_file(char*);
void R(short *data, float* r, int *T);
void *sliding(void*);

FILE *wav_fp;
Header header;
short wav_data[1000000], max_wav_data;
float divx, divy;
unsigned int wav_data_len = 0;
float r_data[K], max_abs_r;
unsigned int backup_cursor[2][161];
char tmp_buff[100];

int main() {
	init_ui();
	read_data_from_file((char*)"xebesvexchef.wav");
	
	setcolor(0);
	setbkcolor(7);
	outtextxy(20, 105, "0");
	sprintf(tmp_buff, "%d", max_wav_data + 5000);
	outtextxy(5, 15, tmp_buff);
	sprintf(tmp_buff, "-%d", max_wav_data + 5000);
	outtextxy(5, 190, tmp_buff);
	
	setlinestyle(0, 0, 1);
	setcolor(10);
	moveto(50, 110);
	divx = wav_data_len / 900.0;
	divy = (max_wav_data + 5000) / 80.0;
	for(int i = 0; i < wav_data_len; i++) {
		lineto(i / divx + 50, 110 - wav_data[i] / divy);
	}

	pthread_t sliding_thread, keyboard_thread;
	pthread_create(&sliding_thread, NULL, sliding, (void*) 1);
	
	getch();
	return 0;
}

void delay(int ms) {
	clock_t start_time = clock();
	while(clock() < start_time + ms);
}

void init_ui() {
#if DEBUG
	printf("Init the UI\n");
#endif
	// init graph
	initwindow(1010, 650);
	setwindowtitle("Wave analyzer");
	setbkcolor(7);
	cleardevice();
	
	setfillstyle(SOLID_FILL, 0);
	
	bar(50, 30, 950, 190);		// wave form
	setcolor(14);	
	setlinestyle(2, 0, 1);
	setbkcolor(0);
	line(50, 70, 950, 70);
	line(50, 110, 950, 110);
	line(50, 150, 950, 150);
	
	bar(50, 210, 490, 390);		// zoom wave form
	bar(510, 210, 950, 390);	// r(x)
	
	bar(50, 410, 950, 590);		// frequency
	setlinestyle(2, 0, 1);
	setcolor(14);
	setbkcolor(0);
	line(50, 500, 950, 500);
	line(50, 455, 950, 455);
	line(50, 545, 950, 545);
	line(150, 410, 150, 590);
	line(250, 410, 250, 590);
	line(350, 410, 350, 590);
	line(450, 410, 450, 590);
	line(550, 410, 550, 590);
	line(650, 410, 650, 590);
	line(750, 410, 750, 590);
	line(850, 410, 850, 590);
	setcolor(0);
	setbkcolor(7);
	outtextxy(5, 585, "0Hz");
	outtextxy(5, 540, "100Hz");
	outtextxy(5, 495, "200Hz");
	outtextxy(5, 450, "300Hz");
	outtextxy(5, 405, "400Hz");
}

void read_data_from_file(char *filename) {
#if DEBUG
	printf("Read data from file '%s'\n", filename);
#endif	
	wav_fp = fopen(filename, "rb");
	// get header
	fread((char*) &header.chunk_id, sizeof(header.chunk_id), 1, wav_fp);
	fread((char*) &header.chunk_size, sizeof(header.chunk_size), 1, wav_fp);
	fread((char*) &header.format, sizeof(header.format), 1, wav_fp);
	fread((char*) &header.subchunk1_id, sizeof(header.subchunk1_id), 1, wav_fp);
	fread((char*) &header.subchunk1_size, sizeof(header.subchunk1_size), 1, wav_fp);
	fread((char*) &header.audio_format, sizeof(header.audio_format), 1, wav_fp);
	fread((char*) &header.num_channels, sizeof(header.num_channels), 1, wav_fp);
	fread((char*) &header.sample_rate, sizeof(header.sample_rate), 1, wav_fp);
	fread((char*) &header.byte_rate, sizeof(header.byte_rate), 1, wav_fp);
	fread((char*) &header.block_align, sizeof(header.block_align), 1, wav_fp);
	fread((char*) &header.bits_per_sample, sizeof(header.bits_per_sample), 1, wav_fp);
	fread((char*) &header.extra_param_size, sizeof(header.extra_param_size), 1, wav_fp);
	fread((char*) &header.subchunk2_id, sizeof(header.subchunk2_id), 1, wav_fp);
	fread((char*) &header.subchunk2_size, sizeof(header.subchunk2_size), 1, wav_fp);

	// get sample data
	short tmp = 0;
	while(fread((char*) &tmp, 2, 1, wav_fp)) {
		wav_data[wav_data_len] = tmp;
		wav_data_len++;
		if(max_wav_data < abs(tmp))
			max_wav_data = abs(tmp);
	}
}

void R(short *x, float* r, int *T) {
	for(int k = 0; k < K; k++) {
		r[k] = 0;
		for(int i = 0; i < N - k; i++) {
			r[k] += x[i] * x[i + k];
		}	
	}
	
	float max_r = 0, max_i = 1;
	max_abs_r = 0;
	for(int k = 0; k < K; k++) {
		if(k > 0 && k < K - 1 && max_r < r[k] && r[k] > r[k - 1] && r[k] > r[k + 1]) {
			max_r = r[k];
			max_i = k;
		}
		if(max_abs_r < fabs(r[k]))
			max_abs_r = fabs(r[k]);
			
	}
	*T = max_i;
}

void *sliding(void *tid) {
	int T;
	float freq = 0;
	
	for(int i = 0; i + N < wav_data_len; i+= N / 2) {
		divx = wav_data_len / 900.0;
		for(int j = 0; j <= 160; j++) {
			if(i > 0) {
				putpixel(50 + (i - N / 2) / divx, j + 30, backup_cursor[0][j]);
				putpixel(50 + (i + N / 2) / divx, j + 30, backup_cursor[1][j]);
			}
			backup_cursor[0][j] = getpixel(50 + i / divx, j + 30);
			backup_cursor[1][j] = getpixel(50 + (i + N) / divx, j + 30);
		}
		setlinestyle(0, 0, 1);
		setcolor(4);
		line(50 + i / divx, 30, 50 + i / divx, 190);
		setcolor(1);
		line(50 + (i + N) / divx, 30, 50 + (i + N) / divx, 190);
		
		R(wav_data + i, r_data, &T);
		
		bar(50, 210, 490, 390);
		setlinestyle(2, 0, 1);
		setcolor(14);
		setbkcolor(0);
		line(50, 300, 490, 300);
		line(50, 255, 490, 255);
		line(50, 345, 490, 345);
		line(160, 210, 160, 390);
		line(270, 210, 270, 390);
		line(380, 210, 380, 390);
		
		bar(510, 210, 950, 390);
		setcolor(14);
		line(510, 300, 950, 300);
		line(510, 255, 950, 255);
		line(510, 345, 950, 345);
		line(620, 210, 620, 390);
		line(730, 210, 730, 390);
		line(840, 210, 840, 390);
		
		// wave form
		setlinestyle(0, 0, 1);
		setcolor(10);
		
		divx = 512.0 / 440;
		divy = 400;
		moveto(50, 300);
		for(int j = 0; j < N; j++) {
			lineto(j / divx + 50, 300 - wav_data[i + j] / divy);
		}
		
		// R(x)
		divx = 256.0 / 440;
		divy = max_abs_r != 0 ? max_abs_r / 90.0 : 1;
		moveto(510, 210);
		for(int j = 0; j < K; j++) {
			lineto(j / divx + 510, 300 - r_data[j] / divy);
		}
		
		setcolor(4);
		line(510 + T / divx, 210, 510 + T / divx, 390);	
		
		// frequency
		setcolor(10);
		divx = wav_data_len / 900.0;
		divy = 400.0 / 180;
		freq = 1.0 / ((float) T / header.sample_rate);
		if(freq < 400)
			circle(50 + i / divx, 590 - freq / divy, 4);
		
		delay(300);
	}
	pthread_exit(NULL);
}

