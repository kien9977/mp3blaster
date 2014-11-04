/* MPEG/WAVE Sound library

	 (C) 1997 by Jung woo-jae */

// Mpegtoraw.cc
// Server which get mpeg format and put raw format.
// Patched by Bram Avontuur to support more mp3 formats and to play mp3's
// with bogus (?) headers.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mpegsound.h"
#include "mpegsound_locals.h"

#ifdef NEWTHREAD
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/time.h>

pth_mutex_t queue_mutex;

Mpegtoraw *decoder;
#define THREAD_TIMES 1 
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include SOUNDCARD_HEADERFILE

#ifdef __cplusplus
}
#endif

#define MY_PI 3.14159265358979323846
#undef DEBUG

extern void debug(const char*);

#ifdef NEWTHREAD
void
Mpegtoraw::settotalbuffers(short amount)
{
	short bla=0;
#if 0
	int  shm_id; /* Shared memory ID */
	do{
		shm_id=shmget(IPC_PRIVATE,sizeof(mpeg_buffer)*amount,IPC_CREAT | IPC_EXCL |
			0666);
		amount-=10;
	} while (shm_id==-1 && (bla+10)>0);
	amount+=10;

	if(amount<=0)
	{
	fprintf(stderr,"Can't create shared memory!\n");
	exit(1);
	}
	buffers=(mpeg_buffer *)shmat(shm_id,NULL,0);
	if(buffers==(mpeg_buffer *)-1)
	{
		perror("shmat");
		exit(1);
		}
		if(shmctl(shm_id,IPC_RMID,NULL))
	{
		perror("shmctl");
		exit(1);
	}
#else
	buffers = (mpeg_buffer*)malloc(amount * sizeof(mpeg_buffer));
	if (!buffers)
	{
		//very blunt.
		fprintf(stderr, "Can't allocate buffer memory!\n");
		exit(1);
	}
#endif

	while(bla != amount)
		buffers[bla++].claimed=0;
	no_buffers=amount;
	free_buffers=no_buffers;
}

int
Mpegtoraw::getfreebuffers(void)
{  
	 return free_buffers;
}

short
Mpegtoraw::request_buffer(void)
{
	short temp=0;
	for(temp=0;temp!=no_buffers;temp++)
	{
		if(!buffers[temp].claimed)
		{
			buffers[temp].claimed=1;
			free_buffers--;
			return temp;
		}
	}
	return -1;
}

void
Mpegtoraw::release_buffer(short index)
{
	 if(index>=no_buffers) return;
	 buffers[index].claimed=0;
	 free_buffers++;
}

void
Mpegtoraw::queue_buf(short index)
{
	 buffer_node *bla;
	 buffer_node *search;
	 search=queue;
	 bla=(buffer_node *)malloc(sizeof(buffer_node));
	 if(!bla)
	 {
			seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
			return;
	 }
	 bla->index=index;
	 bla->next=NULL;

	 if(search==NULL)
	 {
				queue=bla;
	return;
	 }
	 while(search->next!=NULL)
			search=search->next;
	 search->next=bla;
	 return;
}

void
Mpegtoraw::dequeue_first(void)
{
	buffer_node *temp;
	temp=queue;
	if(!temp) 
	{
		 return;
	}
	release_buffer(queue->index);
	queue=queue->next;
	free(temp);
}

void
Mpegtoraw::dequeue_buf(short index)
{
	 buffer_node *temp;
	 buffer_node *temp2;
	 temp=queue;
	 if(temp==NULL)
	 {
			 return;
	 }
	 if(temp->index==index)
	 {
					release_buffer(index);
		queue=queue->next;
		free(temp);
	 }
	 else
	 {
		while(temp!=NULL&&temp->next!=NULL&&temp->next->index!=index)
				 temp=temp->next;
		if(temp==NULL||temp->next==NULL)
		{
			 return;
		}
		release_buffer(temp->next->index); 
		temp2=temp->next;
		temp->next=temp->next->next;
		free(temp2);
	 }
}

void Mpegtoraw::threadplay(void *bla)
{
	struct mpeg_buffer *bufje;
	short int sound_buf[RAWDATASIZE*THREAD_TIMES];
	int cnt;
	int offset;
	bla=bla;
	int tellertje=0;
	bufje=NULL; /* So gcc will not complain. Pfft. */
	if(tellertje);
	/* Enable other threads to kill us */
	/* But only if we feel like it! */
	pth_cancel_state(PTH_CANCEL_ENABLE|PTH_CANCEL_DEFERRED, NULL);

	/* Yes, I know that these are the defaults at this particular point
		 in time/space. Linux and GNU-libc are doing a fine job in changing
		 defaults and compliances these days, though. */
	while(1)
	{
		pth_cancel_point(); /* Are we "requested" to sod off and die? */
		pth_mutex_acquire(&queue_mutex, FALSE, NULL); /* Enter critical zone */
		if(!player_run || !queue)
		{
			pth_mutex_release(&queue_mutex); /* Leave critical zone */
			USLEEP(100000);  //implicit pth_yield (I hope)
			/* Player is either paused, or has a buffer
				 underrun.Pause 100 msec. */
		}
		else
		{
			for(cnt=0,offset=0;(queue);cnt++)
			{
				bufje=get_buffer(queue->index);
				if(offset+bufje->framesize>THREAD_TIMES*(RAWDATASIZE*2))
				{
					break;
				}
				//amplify(bufje->data,bufje->framesize);
				memcpy(&(sound_buf[offset]),bufje->data,bufje->framesize);
				/* framesize is in bytes !*/
				offset+=bufje->framesize/sizeof(short int);
				/* But the sound buffer are short ints.. */

				//framesize=bufje->framesize;
				/* Prevent thread from blocking while in critical zone! */
				currentframe=bufje->framenumber;
				dequeue_first();
			}
			pth_mutex_release(&queue_mutex); /* Leave critical zone */

			/* So, the actual sound output is done -outside- the critical
				 zone. This means that the data is copied once too much,
				 but it prevents the decoding thread from blocking because
				 the playing thread is blocked by the sound hardware while 
				 in the critical zone.*/
			player->putblock(sound_buf,offset*sizeof(short int));
			pth_yield(NULL);
			//USLEEP(100); /* Let's be nice */
//			USLEEP(1);
		}
	}
	/* The playing thread never quits */
}

void *tp_hack(void *bla)
{
	 decoder->threadplay(bla);

	 /* The moron who decided that this should be a C++ class should be shot,
			if he hasn't been by now already.. */
	 return NULL;
}

struct mpeg_buffer *
Mpegtoraw::get_buffer(short index)
{
	return &buffers[index];
}

void
Mpegtoraw::createplayer(void)
{
	//struct sched_param param;
	//int policy;
	pth_attr_t attr;
	if(play_thread)
		return;
	//pthread_create(&(this->play_thread),NULL,&(tp_hack),NULL);
	attr = pth_attr_new();
	pth_attr_set(attr, PTH_ATTR_PRIO, PTH_PRIO_MAX);
	this->play_thread = pth_spawn(attr, tp_hack, NULL);
#if 0
	pthread_getschedparam(this->play_thread,&policy,&param);
	do
	{
		++(param.sched_priority);
	}
	while (!pthread_setschedparam(this->play_thread,policy,&param))
		;
#endif
}

void
Mpegtoraw::continueplaying(void)
{
	this->player_run=1;
}

void
Mpegtoraw::abortplayer(void)
{
	pth_cancel(play_thread);
	if(!play_thread)
		return;
	if(!pth_join(play_thread,NULL)) /* Wait for the playthread to die! */
	{
		perror("pthread_join");
		exit(1);
	}
	play_thread=(pth_t)NULL;
}

void
Mpegtoraw::pauseplaying(void)
{
	//why would you abort the player here??
	//player->abort();
	this->player_run=0;
}

void Mpegtoraw::setdecodeframe(int framenumber)
{
	int pos=0;
	char d[100];

	sprintf(d, "Framenumber = %d\n", framenumber);
	debug(d);

	if(frameoffsets==NULL)return;
	if(framenumber<=0)
	{
		pos=frameoffsets[0];
		framenumber=0;
	}
	else
	{
		if (framenumber >= totalframe)
			framenumber=totalframe-1;

		pos = frameoffsets[framenumber];

		if(pos==0)
		{
			int i;

			sprintf(d,"Bumped into an unset frame position (%d)!\n", framenumber);
			debug(d);
			for(i=framenumber-1;i>0;i--)
				if(frameoffsets[i]!=0)break;

			loader->setposition(frameoffsets[i]);

			sprintf(d,"Found first initialized frame at %d bytes (framenumber %d)\n", 
				loader->getposition(), i);
			debug(d);

			while( i < framenumber )
			{
				loadheader();
				i++;
				frameoffsets[i]=loader->getposition();
			}
			pos=frameoffsets[framenumber];
		}
	}

	loader->setposition(pos);
	decodeframe=framenumber;
}

void Mpegtoraw::makeroom(int size)
{
	int clean;
	int queuesize;
	buffer_node *temp,*temp2;
	if(free_buffers >= size) /* No need to throw away things ! */
		return;
	clean=size-free_buffers;
	if(spare)	/* Begin releasing from spare buffer */
	{
		queuesize=0;
		temp=spare;

		while(temp)
		{
			queuesize++;
			temp=temp->next;
		}
		
		temp = spare;

		if(clean <= queuesize) 
		{
			/* It fits !*/
			queuesize-=clean;
			clean=0;

			while(queuesize-1)
			{
				temp=temp->next;
				queuesize--;
			}
			temp2=temp->next;
			temp->next=NULL;
			temp=temp2;
		}
		else
		{
			clean-=queuesize;
			spare=NULL;
		}

		while(temp)
		{
			release_buffer(temp->index);
			temp2=temp;
			temp=temp->next;
			free(temp2);
		} 
	}
	if(clean&&queue)	/* Begin releasing from spare buffer */
	{
		queuesize=0;
		temp=queue;

		while(temp)
		{
			queuesize++;
			temp=temp->next;
		}

		temp = queue;

		if(clean <= queuesize) 
		{
			/* It fits !*/
			queuesize-=clean;
			clean=0;

			while(queuesize-1)
			{
				temp=temp->next;
				queuesize--;
			}

			temp2=temp->next;
			temp->next=NULL;
			temp=temp2;
		}
		else
		{
			clean-=queuesize;
			queue=NULL;
		}

		while(temp)
		{
			release_buffer(temp->index);
			temp2=temp;
			temp=temp->next;
			free(temp2);
		} 
	}
}
#endif

Mpegtoraw::Mpegtoraw(Soundinputstream *loader,Soundplayer *player)
{
	__errorcode=SOUND_ERROR_OK;
	frameoffsets=NULL;

	forcetomonoflag=0;
	downfrequency=0;

	this->loader=loader;
	this->player=player;
#ifdef NEWTHREAD
	skip=5;
	//pre_amp=10;
	this->buffers=NULL;
	this->settotalbuffers(1000);
	this->queue=NULL;
	this->spare=NULL;
	decoder=this;
	this->player_run=0;
	pth_mutex_init(&queue_mutex);
	play_thread=(pth_t)NULL;
	createplayer();
	continueplaying();
#endif
}

Mpegtoraw::~Mpegtoraw()
{
	if(frameoffsets)delete [] frameoffsets;
#ifdef NEWTHREAD
	buffer_node *temp;
	abortplayer(); 
	/*
	if(pthread_mutex_destroy(&queue_mutex))
	{
		 perror("pthread_mutex_destroy");
		 exit(1);
	}
	*/
	//shmdt((char *)buffers);
	if (buffers)
	{
		delete[] buffers;
		buffers = NULL;
	}
	while(queue)
	{
		 temp=queue;
	 queue=queue->next;
	 free(temp);
	}
	while(spare)
	{
		 temp=spare;
	 spare=spare->next;
	 free(temp);
	}
#endif
}

#ifndef WORDS_BIGENDIAN
#define _KEY 0
#else
#define _KEY 3
#endif

int Mpegtoraw::getbits(int bits)
{
	union
	{
		char store[4];
		int current;
	}u;
	int bi;

	if(!bits)return 0;

	u.current=0;
	bi=(bitindex&7);
	u.store[_KEY]=buffer[bitindex>>3]<<bi;
	bi=8-bi;
	bitindex+=bi;

	while(bits)
	{
		if(!bi)
		{
			u.store[_KEY]=buffer[bitindex>>3];
			bitindex+=8;
			bi=8;
		}

		if(bits>=bi)
		{
			u.current<<=bi;
			bits-=bi;
			bi=0;
		}
		else
		{
			u.current<<=bits;
			bi-=bits;
			bits=0;
		}
	}
	bitindex-=bi;

	return (u.current>>8);
}

void Mpegtoraw::setforcetomono(short flag)
{
	forcetomonoflag=flag;
}

void Mpegtoraw::setdownfrequency(int value)
{
	downfrequency=0;
	if(value)downfrequency=1;
}

short Mpegtoraw::getforcetomono(void)
{
	return forcetomonoflag;
}

int Mpegtoraw::getdownfrequency(void)
{
	return downfrequency;
}

int  Mpegtoraw::getpcmperframe(void)
{
	int s;

	s=32;
	if(layer==3)
	{
		s*=18;
		if(version==0)s*=2;
	}
	else
	{
		s*=SCALEBLOCK;
		if(layer==2)s*=3;
	}

	return s;
}

#ifdef NEWTHREAD
inline void Mpegtoraw::flushrawdata(short index)
{
	 struct mpeg_buffer *bufje;
	 bufje=get_buffer(index);
	 bufje->framenumber=decodeframe;
	 bufje->framesize=rawdataoffset<<1;
	 memcpy(bufje->data,rawdata,bufje->framesize);
	 queue_buf(index);
	rawdataoffset=0;
}
#else
inline void Mpegtoraw::flushrawdata(void)
#ifdef PTHREADEDMPEG
{
	if(threadflags.thread)
	{
		if(((threadqueue.tail+1)%threadqueue.framenumber)==threadqueue.head)
		{
			while(((threadqueue.tail+1)%threadqueue.framenumber)==threadqueue.head)
	USLEEP(200);
		}
		memcpy(threadqueue.buffer+(threadqueue.tail*RAWDATASIZE),rawdata,
		 RAWDATASIZE*sizeof(short int));
		threadqueue.sizes[threadqueue.tail]=(rawdataoffset<<1);
		
		if(threadqueue.tail>=threadqueue.frametail)
			threadqueue.tail=0;
		else threadqueue.tail++;
	}
	else
	{
		if (!suppress_sound)
			player->putblock((char *)rawdata,rawdataoffset<<1);
		currentframe++;
	}
	rawdataoffset=0;
}
#else
{
	if (!suppress_sound)
		player->putblock((char *)rawdata,rawdataoffset<<1);
	currentframe++;
	rawdataoffset=0;
}
#endif
#endif /* NEWTHREAD */

typedef struct
{
	char *songname;
	char *artist;
	char *album;
	char *year;
	char *comment;
	unsigned char genre;
}ID3;

static void strman(char *str,int max)
{
	int i;

	str[max]=0;

	for(i=max-1;i>=0;i--)
		if(((unsigned char)str[i])<26 || str[i]==' ')str[i]=0; else break;
}

inline void parseID3(Soundinputstream *fp,ID3 *data)
{
	int tryflag=0;

	data->songname[0]=0;
	data->artist	[0]=0;
	data->album		[0]=0;
	data->year		[0]=0;
	data->comment [0]=0;
	data->genre=255;

	fp->setposition(fp->getsize()-128);

	for(;;)
	{
		if(fp->getbytedirect()==0x54)
		{
			if(fp->getbytedirect()==0x41)
			{
				if(fp->getbytedirect()==0x47)
				{
					char tmp;
					fp->_readbuffer(data->songname,30);strman(data->songname,30);
					fp->_readbuffer(data->artist	,30);strman(data->artist,  30);
					fp->_readbuffer(data->album		,30);strman(data->album,	 30);
					fp->_readbuffer(data->year		, 4);strman(data->year,			4);
					fp->_readbuffer(data->comment ,30);strman(data->comment, 30);
					fp->_readbuffer(&tmp,1);
					data->genre = (unsigned char)tmp;
					break;
				}
			}
		}

		tryflag++;
		if (tryflag == 1)
			fp->setposition(fp->getsize() - 125); // for mpd 0.1, Sorry..
		else
			break;
	}

	fp->setposition(0);
}

// Convert mpeg to raw
// Mpeg header class
bool Mpegtoraw::initialize(char *filename)
{
	register int i;
	register REAL *s1,*s2;
	REAL *s3,*s4;
	static bool initialized=false;

	if (!filename)
		return false;

	suppress_sound = 0;

	scalefactor       = SCALE;
	calcbufferoffset  = 15;
	currentcalcbuffer = 0;

	s1 = calcbufferL[0]; s2 = calcbufferR[0];
	s3 = calcbufferL[1]; s4 = calcbufferR[1];
	for(i=CALCBUFFERSIZE-1;i>=0;i--)
	{
		calcbufferL[0][i] = calcbufferL[1][i] =
		calcbufferR[0][i] = calcbufferR[1][i] = 0.0;
	}

	if(!initialized)
	{
		for(i=0;i<16;i++)hcos_64[i]=1.0/(2.0*cos(MY_PI*double(i*2+1)/64.0));
		for(i=0;i< 8;i++)hcos_32[i]=1.0/(2.0*cos(MY_PI*double(i*2+1)/32.0));
		for(i=0;i< 4;i++)hcos_16[i]=1.0/(2.0*cos(MY_PI*double(i*2+1)/16.0));
		for(i=0;i< 2;i++)hcos_8 [i]=1.0/(2.0*cos(MY_PI*double(i*2+1)/ 8.0));
		hcos_4=1.0/(2.0*cos(MY_PI*1.0/4.0));
		initialized=true;
	}

	layer3initialize();

	currentframe=decodeframe=0;
	is_vbr = 0;
	int headcount = 0, first_offset = 0, loopcount = 0;
	sync();
	while (headcount < 1 && geterrorcode() != SOUND_ERROR_FINISH)
	{
		int fileseek = loader->getposition();

		if (loadheader())
		{
			if (!headcount)
				first_offset = fileseek;
			headcount++;
		}
		else
		{
			headcount = 0; //reset counter when bad header's found.
			loader->setposition(fileseek + 2); //skip 2 bytes or else this loop
																				 //might never end
		}
		loopcount++;
	}

	if(headcount > 0)
	{
		totalframe = (loader->getsize() + framesize - 1) / framesize;
	}
	else totalframe=0;

	if(frameoffsets)
		delete[] frameoffsets;

	songinfo.name[0]=0;
	if (totalframe > 0)
	{
		frameoffsets=new int[totalframe];
		for(i=totalframe-1;i>=0;i--)
			frameoffsets[i]=0;

		//Bram Avontuur: I added this. The original author figured an mp3
		//would always start with a valid header (which is false if  there's a
		//sequence of 3 0xf's that's not part of a valid header)
		frameoffsets[0] = first_offset;
		//till here. And a loader->setposition later in this function.

		{
			ID3 data;

			data.songname=songinfo.name;
			data.artist  =songinfo.artist;
			data.album	 =songinfo.album;
			data.year		 =songinfo.year;
			data.comment =songinfo.comment;
			//data.genre	 =songinfo.genre;
			parseID3(loader,&data);
		songinfo.genre = data.genre;
		}
	}
	else frameoffsets=NULL;

#ifndef NEWTHREAD
#ifdef PTHREADEDMPEG
	threadflags.thread=false;
	threadqueue.buffer=NULL;
	threadqueue.sizes=NULL;
#endif
#endif /* NEWTHREAD */

	if (totalframe)
	{
		//Calculate length of [vbr] mp3.
		if (1) 
		{
			char d[100];

			// Get frame offsets, by calculating all offsets of the headers.
			sprintf(d, "Totalframe(0): %d\n", totalframe);
			debug(d);

			setframe(totalframe - 1);

			// Cut off last frame until there is an offset change
			int i;
			int lastOffset = frameoffsets[totalframe - 1];
			for(i = totalframe - 2;i > 0;i--)
			{
				if(frameoffsets[i] != lastOffset)
				{
					break;
				}
			}
			totalframe = i;

			sprintf(d, "Totalframe(1): %d\n", totalframe);
			debug(d);

			// Reset position
			setframe(0);
		}
		else
			loader->setposition(first_offset);
		seterrorcode(SOUND_ERROR_OK);
	}

	return (totalframe != 0);
}

#ifdef NEWTHREAD
void Mpegtoraw::setframe(int framenumber)
{
	buffer_node *temp;
	abortplayer(); 
	player->abort();
	if(queue)
	{
		if(buffers[queue->index].framenumber < framenumber)
		{
			while(queue && buffers[queue->index].framenumber < framenumber)
			{
					dequeue_first();
			}
		}
		else
		{
			makeroom(buffers[queue->index].framenumber - framenumber);
	
			if(spare)
			{
				 temp=queue;
				 while(temp->next)
					 temp=temp->next;
				 temp->next=spare;
			}
			spare=queue;
			queue=NULL;
		}
	}
	if(!queue) /* Hint: This is -not- the same as an 'else' from the previous if*/
	{
		//can't skip 2 frames because initialization of frameoffsets[] in 
		//setdecodeframe will fail to set the last 2 indices then.
		//setdecodeframe(framenumber-2);
		//skip=2;
		setdecodeframe(framenumber);
		skip=0;
	}
	createplayer();
}
#else /* NEWTHREAD */
void Mpegtoraw::setframe(int framenumber)
{
	int pos=0;
	char d[100];

	if (frameoffsets==NULL)
		return;

	if (framenumber==0)
		pos = frameoffsets[0];
	else
	{
		if (framenumber >= totalframe)
			framenumber = totalframe - 1;
			
		pos=frameoffsets[framenumber];
		
		if(pos==0)
		{
			int i;

			for (i = framenumber - 1; i > 0; i--)
			{
				if (frameoffsets[i] != 0)
					break;
			}

			loader->setposition(frameoffsets[i]);
			
			sprintf(d, "Found first offset at %d (%d/%d)\n", loader->getposition(),
				i, framenumber);
			debug(d);

			while(i < framenumber)
			{
				i++;
				loadheader();
				frameoffsets[i] = loader->getposition();
			}
			pos = frameoffsets[framenumber];
		}
	}

	clearbuffer();
	loader->setposition(pos);
	decodeframe=currentframe=framenumber;
}
#endif /* NEWTHREAD */

void Mpegtoraw::clearbuffer(void)
{
#ifndef NEWTHREAD
#ifdef PTHREADEDMPEG
	if(threadflags.thread)
	{
		threadflags.criticalflag=false;
		threadflags.criticallock=true;
		while(!threadflags.criticalflag)USLEEP(1);
		threadqueue.head=threadqueue.tail=0;
		threadflags.criticallock=false;
	}
#endif
#endif /* NEWTHREAD */
	player->abort();
	player->resetsoundtype();
}

//find a valid frame. If one is found, read the frame (so the filepointer
//points to the next frame). If an invalid frame is found, the filepointer
//points to the first or second byte after a sequence of 0xff 0xf? bytes.
bool Mpegtoraw::loadheader(void)
{
	register int c;
	bool flag;
	sync();

// Synchronize
	flag=false;
	do
	{

		if ( (c = loader->getbytedirect()) < 0 )
			break;

		if (c == 0xff)
		{
			while (!flag)
			{
				if ( (c = loader->getbytedirect()) < 0)
				{
					flag=true;
					break;
				}
				if ( (c&0xf0) == 0xf0 )
				{
					flag=true;
					break;
				}
				else if ( c!= 0xff )
					break;
			}
		}
	} while (!flag);

	if (c < 0)
		return seterrorcode(SOUND_ERROR_FINISH);

// Analyzing
	c &= 0xf; /* c = 0 0 0 0 bit13 bit14 bit15 bit16 */
	protection = c&1; /* bit16 */
	layer = 4 - ( (c>>1)&3 ); /* bit 14 bit 15: 1..4, 4=invalid */
	if (layer == 4)
		return seterrorcode(SOUND_ERROR_BAD);

	version = (_mpegversion)((c>>3)^1); /* bit 13 */

	c = ((loader->getbytedirect()))>>1; /* c = 0 bit17 .. bit23 */
	padding = (c&1);
	c >>= 1;

	frequency = (_frequency)(c&3);
	c >>= 2;

	if (frequency > 2)
		return seterrorcode(SOUND_ERROR_BAD);

	bitrateindex = (int)c;
	if (bitrateindex == 15 || !bitrateindex)
		return seterrorcode(SOUND_ERROR_BAD);

	c = ((unsigned int)(loader->getbytedirect()))>>4;
	/* c = 0 0 0 0 bit25 bit26 bit27 bit27 bit28 */
	extendedmode = c&3;
	mode = (_mode)(c>>2);

// Making information
	inputstereo = ( (mode==single) ? 0 : 1 );
	if (forcetomonoflag)
		outputstereo = 0;
	else
		outputstereo = inputstereo;

	channelbitrate = bitrateindex;
	if (inputstereo)
	{
		if (channelbitrate == 4)
			channelbitrate = 1;
		else
			channelbitrate-=4;
	}

	if(channelbitrate == 1 || channelbitrate == 2)
		tableindex = 0;
	else
		tableindex = 1;

	if (layer == 1)
		subbandnumber = MAXSUBBAND;
	else
	{
		if(!tableindex)
		{
			if(frequency==frequency32000)
				subbandnumber=12;
			else
				subbandnumber=8;
		}
		else if (frequency==frequency48000 || 
			(channelbitrate>=3 && channelbitrate<=5))
		{
			subbandnumber=27;
		}
		else
			subbandnumber=30;
	}

	if (mode==single)
		stereobound=0;
	else if(mode==joint)
		stereobound = (extendedmode+1)<<2;
	else
		stereobound = subbandnumber;

	if (stereobound>subbandnumber)
		stereobound = subbandnumber;

	// framesize & slots
	if (layer==1)
	{
		framesize=(12000*bitrate[version][0][bitrateindex])/
							frequencies[version][frequency];
		if(frequency==frequency44100 && padding)framesize++;
		framesize<<=2;
	}
	else
	{
		framesize=(144000*bitrate[version][layer-1][bitrateindex])/
			(frequencies[version][frequency]<<version);
		if(padding)
			framesize++;
		if(layer==3)
		{
			if(version)
				layer3slots = framesize - ((mode==single)?9:17) - (protection?0:2) - 4;
			else
				layer3slots = framesize-((mode==single)?17:32) - (protection?0:2) - 4;
		}
	}
	if(!fillbuffer(framesize-4))
		seterrorcode(SOUND_ERROR_FILEREADFAIL);

	if(!protection)
	{
		getbyte();											// ignore CRC
		getbyte();
	}

	if (loader->eof())
		return seterrorcode(SOUND_ERROR_FINISH);

	return true;
}

/***************************/
/* Playing in multi-thread */
/***************************/
#ifndef NEWTHREAD
#ifdef PTHREADEDMPEG
/* Player routine */
void Mpegtoraw::threadedplayer(void)
{
	while(!threadflags.quit)
	{
		while(threadflags.pause || threadflags.criticallock)
		{
			threadflags.criticalflag=true;
			USLEEP(200);
		}

		if(threadqueue.head!=threadqueue.tail)
		{
			player->putblock(threadqueue.buffer+threadqueue.head*RAWDATASIZE,
								 threadqueue.sizes[threadqueue.head]);
			currentframe++;
			if(threadqueue.head==threadqueue.frametail)
	threadqueue.head=0;
			else threadqueue.head++;
		}
		else
		{
			if(threadflags.done)break;	// Terminate when done
			USLEEP(200);
		}
	}
	threadflags.thread=false;
}

static void *threadlinker(void *arg)
{
	((Mpegtoraw *)arg)->threadedplayer();

	return NULL;
}

bool Mpegtoraw::makethreadedplayer(int framenumbers)
{
	threadqueue.buffer=
		(short int *)malloc(sizeof(short int)*RAWDATASIZE*framenumbers);
	if(threadqueue.buffer==NULL)
		seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
	threadqueue.sizes=(int *)malloc(sizeof(int)*framenumbers);
	if(threadqueue.sizes==NULL)
		seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
	threadqueue.framenumber=framenumbers;
	threadqueue.frametail=framenumbers-1;
	threadqueue.head=threadqueue.tail=0;


	threadflags.quit=threadflags.done=
	threadflags.pause=threadflags.criticallock=false;

	threadflags.thread=true;
	if(pthread_create(&thread,0,threadlinker,this))
		seterrorcode(SOUND_ERROR_THREADFAIL);

	return true;
}

void Mpegtoraw::freethreadedplayer(void)
{
	threadflags.criticallock=
	threadflags.pause=false;
	threadflags.done=true;							 // Terminate thread player
	while(threadflags.thread)USLEEP(10); // Wait for done...
	if(threadqueue.buffer)
	{
		free(threadqueue.buffer);
		threadqueue.buffer = NULL;
	}
	if(threadqueue.sizes)
	{
		free(threadqueue.sizes);
		threadqueue.sizes = NULL;
	}
}




void Mpegtoraw::stopthreadedplayer(void)
{
	threadflags.quit=true;
};

void Mpegtoraw::pausethreadedplayer(void)
{
	threadflags.pause=true;
};

void Mpegtoraw::unpausethreadedplayer(void)
{
	threadflags.pause=false;
};


bool Mpegtoraw::existthread(void)
{
	return threadflags.thread;
}

int  Mpegtoraw::getframesaved(void)
{
	if(threadqueue.framenumber)
		return
			((threadqueue.tail+threadqueue.framenumber-threadqueue.head)
			 %threadqueue.framenumber);
	return 0;
}

#endif
#endif /* NEWTHREAD */



// Convert mpeg to raw
bool Mpegtoraw::run(int frames)
{
#ifdef NEWTHREAD
	short buffer_number = -1;
	buffer_node *temp;
#endif
	clearrawdata();
	if(frames<0)lastfrequency=0;

	for(;frames;frames--)
	{
#ifdef NEWTHREAD
		if(spare) /* There are frames in the spare buffer! */
		{
	pth_mutex_acquire(&queue_mutex,FALSE,NULL);
				if(buffers[spare->index].framenumber < decodeframe)
				{
		 /* Houston, we have a problem! */
		 while(spare && buffers[spare->index].framenumber < decodeframe)
		 {
			 release_buffer(spare->index);
			 temp=spare;
			 spare=spare->next;
			 free(temp);
		 }
	pth_mutex_release(&queue_mutex);
		 continue;
				}
				if(buffers[spare->index].framenumber==decodeframe)
				{
					 while(spare && buffers[spare->index].framenumber==decodeframe)
					 {
						 queue_buf(spare->index);
			 spare=spare->next;
			 decodeframe++;
					 }
					 setdecodeframe(decodeframe-5); /* Not that stupid as it seems */
		 skip=4;
				}  
	pth_mutex_release(&queue_mutex);
		} /* If(spare) */
		if(!skip)
		{
			 pth_mutex_acquire(&queue_mutex,FALSE,NULL);
			 buffer_number=request_buffer();
			 pth_mutex_release(&queue_mutex);
			 if(buffer_number==-1) /* No free buffers available */
					break;
		}
		else
#endif
		if(totalframe>0)
		{
			if(decodeframe<totalframe)
	frameoffsets[decodeframe]=loader->getposition();
		}

		if(loader->eof())
		{
#ifdef NEWTHREAD
			if(!skip)
			{
				pth_mutex_acquire(&queue_mutex,FALSE,NULL);
				release_buffer(buffer_number);
				pth_mutex_release(&queue_mutex);
			}
#endif
			seterrorcode(SOUND_ERROR_FINISH);
			break;
		}
		if(loadheader()==false)
		{
			if (geterrorcode() == SOUND_ERROR_BAD || geterrorcode() == 
				SOUND_ERROR_BADHEADER)
			{
				suppress_sound = 0;
				continue;
			}
			else
				break;
		}
		else if (suppress_sound)
			suppress_sound--;

		register int resetsound = 0;

		if(frequency!=lastfrequency)
		{
			if(lastfrequency>0)
			seterrorcode(SOUND_ERROR_BAD);
			lastfrequency=frequency;
			resetsound = 1;
		}
		if (outputstereo != laststereo)
		{
			resetsound = 1;
			laststereo = outputstereo;
		}

		//TODO: AFMT_S16_NE => 8 / 16 bits depending on user settings
		if (resetsound || frames < 0)
			player->setsoundtype(outputstereo,AFMT_S16_NE,
				frequencies[version][frequency]>>downfrequency);

		if(frames<0)
		{
			frames=-frames;
			totaltime = (totalframe * framesize) / (getbitrate() * 125);
		}

		decodeframe++;

		if		 (layer==3)extractlayer3();
		else if(layer==2)extractlayer2();
		else if(layer==1)extractlayer1();

#ifdef NEWTHREAD
		if(skip)
		{
			skip--;
			rawdataoffset=0;
		}
		else
		{
			pth_mutex_acquire(&queue_mutex,FALSE,NULL);
			flushrawdata(buffer_number);
			pth_mutex_release(&queue_mutex);
		}
#else
		flushrawdata();
#endif
		if(player->geterrorcode())seterrorcode(geterrorcode());
	}

#ifdef NEWTHREAD
	if (geterrorcode() != SOUND_ERROR_OK && queue)
		seterrorcode(SOUND_ERROR_OK);

	if (getfreebuffers() < 10) //buffer is quite full, let's wait a bit
		USLEEP(10000);
#endif

	if (geterrorcode()==SOUND_ERROR_OK ||
		geterrorcode() == SOUND_ERROR_BAD ||
		geterrorcode() == SOUND_ERROR_BADHEADER)
		return true;
	else
		return false;
}
