#include <stdio.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <winbgim.h>

#define DEBUG 0
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
void draw_wave_form();
void draw_zoom_wave_form(short*, int);
void draw_r(float*, int);
int R(short *data, float* r);
void set_cursor(int id, int pos, int color);
void check_keyboard();
void *sliding(void*);

FILE *wav_fp;
Header header;
short wav_data[1000000], max_wav_data;
float divx, divy;
unsigned int wav_data_len = 0;
float r_data[K], max_abs_r;
int sliding_finish = 0;
unsigned int backup_cursor[2][161];
int cursor_pos[2] = {-1, -1};
char tmp_buff[100];

int main() {
	init_ui();
	read_data_from_file((char*)"xebesvexchef.wav");
	draw_wave_form();
	
	// create a seperate thread to slide through all wav data 
	pthread_t sliding_thread;
	pthread_create(&sliding_thread, NULL, sliding, (void*) 1);
	
	// check keyboard to move the cursor over wave form
	check_keyboard();
	
	// exit the program with success code
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

int R(short *x, float* r) {
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
	}
	return max_i;
}

void draw_wave_form() {
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
}

void draw_zoom_wave_form(short *data, int size) {
	// redraw the view for zoom wave form
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
	
	short max_wave_data = 0;
	for(int i = 0; i < size; i++) {
		if(max_wave_data < abs(data[i]))
			max_wave_data = abs(data[i]);
	}
	
	// zoom wave form
	setlinestyle(0, 0, 1);
	setcolor(10);
	
	float divx = size / 440.0;
	float divy = (max_wave_data + 1000) / 90.0;
	moveto(50, 300);
	for(int i = 0; i < size; i++) {
		lineto(i / divx + 50, 300 - data[i] / divy);
	}
}

void draw_r(float *r_data, int T) {
	// redraw the view for R(x)
	bar(510, 210, 950, 390);
	setlinestyle(2, 0, 1);
	setcolor(14);
	setbkcolor(0);
	line(510, 300, 950, 300);
	line(510, 255, 950, 255);
	line(510, 345, 950, 345);
	line(620, 210, 620, 390);
	line(730, 210, 730, 390);
	line(840, 210, 840, 390);
	
	float max_r = 0;
	for(int i = 0; i < K; i++) {
		if(max_r < fabs(r_data[i]))
			max_r = fabs(r_data[i]);
	}
	
	// R(x)
	setlinestyle(0, 0, 1);
	setcolor(10);
	float divx = 256.0 / 440;
	float divy = max_r != 0 ? max_r / 90.0 : 1;
	moveto(510, 210);
	for(int i = 0; i < K; i++) {
		lineto(i / divx + 510, 300 - r_data[i] / divy);
	}
	
	setcolor(4);
	line(510 + T / divx, 210, 510 + T / divx, 390);
}

void *sliding(void *tid) {
	int T;
	float freq = 0;
	
	for(int i = 0; i + N < wav_data_len; i+= N / 2) {
		set_cursor(0, i, 4);
		set_cursor(1, i + N, 1);
		
		T = R(wav_data + i, r_data);
		
		draw_zoom_wave_form(wav_data + i, 512);	
		draw_r(r_data, T);
		
		// frequency
		setcolor(10);
		divx = wav_data_len / 900.0;
		divy = 400.0 / 180;
		freq = 1.0 / ((float) T / header.sample_rate);
		if(freq < 400)
			circle(50 + i / divx, 590 - freq / divy, 4);
		
		delay(300);
	}
	
	set_cursor(0, -1, 0);
	set_cursor(1, -1, 0);
	sliding_finish = 1;
	pthread_exit(NULL);		// finish the thread
}

void check_keyboard() {
	float divx = wav_data_len / 900.0;
	
	while(1) {
		char key = getch();

		if(key == 't' && sliding_finish == 1) {
			if(cursor_pos[0] > 0) 
				set_cursor(0, cursor_pos[0] - divx, 4);
		}
		else if(key == 'T' && sliding_finish == 1) {
			if(cursor_pos[1] > cursor_pos[0])
				set_cursor(1, cursor_pos[1] - divx, 1);
		}
		else if(key == 'p') {
			if(cursor_pos[0] < cursor_pos[1])
				set_cursor(0, cursor_pos[0] + divx, 4);
		}
		else if(key == 'P') {
			if(cursor_pos[1] + divx < wav_data_len)
				set_cursor(1, cursor_pos[1] + divx, 1);
		}
		else if(key == 'i') {
			draw_zoom_wave_form(wav_data + cursor_pos[0], cursor_pos[1] - cursor_pos[0] + 1);
		}
		else if(key == 'x') {
			break;
		}
	}
}

void set_cursor(int id, int pos, int color) {
	float divx = wav_data_len / 900.0;
	for(int i = 0; i <= 160; i++) {
		if(cursor_pos[id] >= 0) {
			putpixel(50 + cursor_pos[id] / divx, i + 30, backup_cursor[id][i]);
		}
		if(pos >= 0) {
			backup_cursor[id][i] = getpixel(50 + pos / divx, i + 30);
		}
	}
	cursor_pos[id] = pos;
	
	if(pos >= 0) {
		setlinestyle(0, 0, 1);
		setcolor(color);
		line(50 + pos / divx, 30, 50 + pos / divx, 190);
	}
}
