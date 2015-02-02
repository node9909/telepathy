// gcc -std=gnu99 -Wall -export-dynamic -g eegdata.c eegdata_callbacks.c kiss_fft/tools/kiss_fftr.c kiss_fft/kiss_fft.c -o eegdata  -lbiosig -lcholmod -lm -Ikiss_fft -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_objdetect `pkg-config --cflags gtk+-3.0 --libs gtk+-3.0 `
#include <stdlib.h>
#include <biosig.h>
#include <complex.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h> /* Error number definitions */

#include "eegdata.h"





GtkWidget* create_window (void)
{
	GtkBuilder *builder;
	GError* error = NULL;
	char name[16];
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, UI_FILE, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	gtk_builder_connect_signals (builder, NULL);
	GtkWidget *window= GTK_WIDGET (gtk_builder_get_object (builder, "window1"));
//	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	video = GTK_WIDGET (gtk_builder_get_object (builder, "drawingarea1"));
	signalview = GTK_WIDGET (gtk_builder_get_object (builder, "drawingarea2"));
	devicechoice=GTK_WIDGET(gtk_builder_get_object (builder, "radiobutton1"));
	startbutton=GTK_WIDGET(gtk_builder_get_object( builder,"togglebutton1"));
	chan1check=GTK_WIDGET(gtk_builder_get_object( builder,"checkbutton1"));
	chan2check=GTK_WIDGET(gtk_builder_get_object( builder,"checkbutton2"));
	chan3check=GTK_WIDGET(gtk_builder_get_object( builder,"checkbutton3"));
	chan4check=GTK_WIDGET(gtk_builder_get_object( builder,"checkbutton4"));
	timecheck=GTK_WIDGET(gtk_builder_get_object( builder,"radiobutton3"));
//	freqcheck=GTK_WIDGET(gtk_builder_get_object( builder,"radiobutton4"));
	rangespin=GTK_WIDGET(gtk_builder_get_object( builder,"spinbutton1"));

	savedialog=GTK_WIDGET(gtk_builder_get_object( builder,"window2"));
	for(unsigned int i=0;i<10;i++){ 
	  sprintf(name,"nameentry%d",i+1);
	  savename[i]=GTK_WIDGET(gtk_builder_get_object( builder,name));
	  gtk_widget_set_name(savename[i],name);
	}
	for(unsigned int i=0;i<10;i++){
	  sprintf(name,"actionentry%d",i+1);
	  saveaction[i]=GTK_WIDGET(gtk_builder_get_object( builder,name));
	  gtk_widget_set_name(saveaction[i],name);
	}
	for(unsigned int i=0;i<10;i++){ 
	  sprintf(name,"savebutton%d",i+1);
	  savebutton[i]=GTK_WIDGET(gtk_builder_get_object( builder,name));
	  gtk_widget_set_name(savebutton[i],name);
	}
	for(unsigned int i=0;i<10;i++){ 
	  sprintf(name,"deletebutton%d",i+1);
	  deletebutton[i]=GTK_WIDGET(gtk_builder_get_object( builder,name));
	  gtk_widget_set_name(deletebutton[i],name);
	}
	for(unsigned int i=0;i<10;i++){ 
	  sprintf(name,"activecheckbutton%d",i+1);
	  activebutton[i]=GTK_WIDGET(gtk_builder_get_object( builder,name));
	  gtk_widget_set_name(activebutton[i],name);
	}
	g_object_unref (builder);
	background= cairo_image_surface_create_from_png ("./grid.png");
	scope = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,SCOPEWIDTH,SCOPEHEIGHT);
	
	return window;
}  

void trace(cairo_t* sc,unsigned short *data){
  double x = SCOPEWIDTH-(PACKETSIZE*XSCALE);
  double y= SCOPEHEIGHT-(data[0]*YSCALE);
  
  cairo_move_to (sc,x, y);
  cairo_set_line_width(sc,2);
  for(int i=0;i<PACKETSIZE;i++){
      y=SCOPEHEIGHT-(data[i+1]*YSCALE);
      x+=XSCALE;
      cairo_line_to (sc,x, y);
  }
  
  
}




void writepackets(unsigned short *rxbuffer){

//shift everything down
  int j=0;
  memmove(channel1,channel1+PACKETSIZE,sizeof(unsigned short)*SAMPLESIZE-PACKETSIZE);
  memmove(channel2,channel2+PACKETSIZE,sizeof(unsigned short)*SAMPLESIZE-PACKETSIZE);
  memmove(channel3,channel3+PACKETSIZE,sizeof(unsigned short)*SAMPLESIZE-PACKETSIZE);
  memmove(channel4,channel4+PACKETSIZE,sizeof(unsigned short)*SAMPLESIZE-PACKETSIZE);
  for(int i=SAMPLESIZE-PACKETSIZE;i<SAMPLESIZE;i++,j+=4){
	channel1[i] = rxbuffer[j+3];//16 bits
	channel2[i] = rxbuffer[j+2];
	channel3[i] = rxbuffer[j+1];
	channel4[i] = rxbuffer[j];
	//	if(!swrite(data, 1, file))perror("write failed:");
    }
    
    
}



void createedf(void){
  
    HDRTYPE	*edfout = NULL;
    char	*filename="./telepathy-eeg.edf";


    edfout = constructHDR(3, 1);
    if (edfout == NULL) {
      perror("constructing header");
      return ;
    }
      
    edfout->TYPE=EDF;

     edfout->CHANNEL[0].PhysMin=0;	/* physical minimum */
     edfout->CHANNEL[0].PhysMax=3.3;	/* physical maximum */
     edfout->CHANNEL[0].DigMin=0;	/* digital minimum */
     edfout->CHANNEL[0].DigMax=4096;	/* digital maximum */
     edfout->CHANNEL[0].Cal=1;		/* gain factor */
     edfout->CHANNEL[0].Off=0;		/* bias */
     edfout->CHANNEL[0].OnOff=1;
     edfout->CHANNEL[0].SPR=1;
     edfout->CHANNEL[0].TOffset=1/SAMPLESIZE;
     sprintf(edfout->CHANNEL[0].Label,"CHANNEL 1");;
 
     edfout->CHANNEL[1].PhysMin=0;	/* physical minimum */
     edfout->CHANNEL[1].PhysMax=3.3;	/* physical maximum */
     edfout->CHANNEL[1].DigMin=0;	/* digital minimum */
     edfout->CHANNEL[1].DigMax=4096;	/* digital maximum */
     edfout->CHANNEL[1].Cal=1;		/* gain factor */
     edfout->CHANNEL[1].Off=0;		/* bias */
     edfout->CHANNEL[1].OnOff=1;
     edfout->CHANNEL[1].SPR=1;
     edfout->CHANNEL[1].TOffset=1/SAMPLESIZE;
     sprintf(edfout->CHANNEL[1].Label,"CHANNEL 2");;

     sprintf(edfout->CHANNEL[2].Label,"EDF Annotations");

    edfout->FLAG.ANONYMOUS=1;
    edfout->SPR = 1;
    edfout->SampleRate=SAMPLESIZE;
//    edfout->EVENT.SampleRate=1;

     mkfifo(filename, 0666);    
    
     edfout = sopen(filename, "w", edfout);
     if (edfout == NULL) {
       perror("opening output file");
       return;
     }

    edfout->NRec=1;
     
//    fakepackets(edfout,SAMPLESIZE);

    sclose(edfout);
    unlink(filename); 
  
  
  
}


void allocate_buffers(void){

  // allocate an "array of arrays" of int
  channel1 = (unsigned short*)malloc( sizeof(unsigned short)*SAMPLESIZE ) ;
  channel2 = (unsigned short*)malloc( sizeof(unsigned short)*SAMPLESIZE ) ;
  channel3 = (unsigned short*)malloc( sizeof(unsigned short)*SAMPLESIZE ) ;
  channel4 = (unsigned short*)malloc( sizeof(unsigned short)*SAMPLESIZE ) ;
  spectrum1 = (unsigned short*)malloc( SAMPLESIZE*sizeof(unsigned short) ) ;
  spectrum2 = (unsigned short*)malloc( SAMPLESIZE*sizeof(unsigned short) ) ;
  spectrum3 = (unsigned short*)malloc( SAMPLESIZE*sizeof(unsigned short) ) ;
  spectrum4 = (unsigned short*)malloc( SAMPLESIZE*sizeof(unsigned short) ) ;
  memset(channel1,0,SAMPLESIZE);
  memset(channel2,0,SAMPLESIZE);
  memset(channel3,0,SAMPLESIZE);
  memset(channel4,0,SAMPLESIZE);

  
}


void read_settings_file(void){
 FILE *settings=NULL;
 settings=fopen(SAVE_FILE,"r");
 if(settings==NULL)return;
 for(unsigned int i=0;i<10;i++){
    fread(&triggers[i],sizeof(TRIGGER),1,settings); 
    gtk_entry_set_text(GTK_ENTRY(savename[i]),triggers[i].name);
    gtk_entry_set_text(GTK_ENTRY(saveaction[i]),triggers[i].action);
   
  }
 fclose(settings);
}

void save_settings_file(void){
 FILE *settings=NULL;
 settings=fopen(SAVE_FILE,"w");
 if(settings==NULL)return;
 for(unsigned int i=0;i<10;i++){
      fwrite(&triggers[i],sizeof(TRIGGER),1,settings); 
 }
 fclose(settings);
}



int main( int   argc,char *argv[]){
    allocate_buffers();
    GtkWidget *window;
    gtk_init(&argc, &argv);
    window = create_window ();
    read_settings_file();
    for(unsigned int i=0;i<10;i++)getpatternimage(i);
    sprintf(mode,"-f");
    gtk_widget_show (window);

    gtk_main();
/*
    while(1){
      gtk_main_iteration_do(FALSE);
      if(controlbits.DATA){
	    getdata();
	    compare_signals();
      }
      if(controlbits.DONE)break;
    }
*/
    return 0;

}
