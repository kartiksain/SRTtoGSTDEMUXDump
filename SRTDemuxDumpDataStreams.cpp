#include <iostream>
#include <gst/gst.h>
#include <glib.h>
#include <glib-object.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <gio/gio.h>

class RecieveSRT
{
    std::string m_ipadd;
    std::string m_portno;
    std::string m_audio_loc;
    std::string m_video_loc;
public:
    
    ~RecieveSRT() { std::cout<<"RecieveSRT Instance Deleted"<<std::endl;}

    RecieveSRT() : m_ipadd("127.0.0.1"), m_portno("8888"), m_audio_loc("./audio/gstaudiodump.raw"), m_video_loc("./video/gstvideodump.raw") {};

    void setIPAddress(std::string inadd)        {m_ipadd=inadd;}
    void setPortNo(std::string inport)          {m_portno=inport;}
    void setAudioLoc(std::string inAudioLoc)    {m_audio_loc=inAudioLoc;}
    void setVideoLoc(std::string inVideoLoc)    {m_video_loc=inVideoLoc;}

    std::string getIPAdd()      const {return m_ipadd;}
    std::string getPortNo()     const {return m_portno;}
    std::string getAudioLoc()   const {return m_audio_loc;}
    std::string getVideoLoc()   const {return m_video_loc;}
};

class GSTPipeline
{
    GstElement *pipeline, *srtsource, *cqueue, *demuxer, *aqueue ,*vqueue , *asink, *vsink;
    GMainLoop *loop;

public:

    ~GSTPipeline() {std::cout<<"delete done"<<setPipelineState(false)<<std::endl;
                    runMainEventLoop(false);
                    }

    GSTPipeline(): loop(nullptr), pipeline(nullptr),srtsource(nullptr),cqueue(nullptr),demuxer(nullptr),aqueue(nullptr),vqueue(nullptr),asink(nullptr),vsink(nullptr) {};

    /* Handler for the pad-added signal */
    static void pad_added_handler (GstElement *src, GstPad *pad, void *data);
    /* Handler SRT Connect or Disconnect */
    static void caller_added_handler (GstElement *src, gint arg0,GSocketAddress *arg1 ,void* user_data);
    static void caller_removed_handler (GstElement *src, gint arg0,GSocketAddress *arg1 ,void * user_data);

    bool gstINIT(); // definition outside class
    void setElementProperties(RecieveSRT *srtInfo); //definition outside class

    /* Adding Elements to One Single Bin */
    void addElementsBin() {
        gst_bin_add_many(GST_BIN (pipeline),srtsource, cqueue, demuxer, aqueue, vqueue , asink , vsink, NULL);
    }

    /* Link Elements that have pads type always */
    void linkStaticElements() {
        gst_element_link_many(srtsource, cqueue, demuxer, NULL);   // SRTSource -> TS DATA Container in Queue -> Send to TS Demuxer Assumption is data is in TS
        gst_element_link_many(aqueue, asink, NULL); // Audio Queue -> AudioSink 
        gst_element_link_many(vqueue, vsink, NULL); // Video Queue -> VideoSink
    }

    /* Link Elements that have pads type sometime */

    void linkDynamicElements() {
    
        /* Connect to the pad-added signal */
        g_signal_connect (demuxer, "pad-added", G_CALLBACK (pad_added_handler), this);
    
        /* Attach Signal connect to SRTSource */
        g_signal_connect (G_OBJECT(srtsource), "caller-added", G_CALLBACK (caller_added_handler), NULL);
        g_signal_connect (G_OBJECT(srtsource), "caller-removed", G_CALLBACK (caller_removed_handler), NULL);    
    }
    bool setPipelineState(bool in) {
        /* Set the pipeline to "playing" state*/
        if(in==true)
        return gst_element_set_state(pipeline, GST_STATE_PLAYING);
        else 
        return gst_element_set_state(pipeline, GST_STATE_NULL);
    }
    void runMainEventLoop(bool state) {
     /* Set Main Event Loop Running to activate all calls and signals from GST */
        if(state==true)
        { 
            g_print("Running...\n");
            g_main_loop_run(loop);
        }
        else
        {
            g_print("Quit...\n");
           g_main_loop_quit(loop);
        }
    }
};

void GSTPipeline::caller_added_handler (GstElement *src, gint arg0,GSocketAddress *arg1 ,gpointer user_data)
{
    g_print("Caller Added to SRT Source of Gstreamer Plugin\n");
}
void GSTPipeline::caller_removed_handler (GstElement *src, gint arg0,GSocketAddress *arg1 ,gpointer user_data)
{
    g_print("Caller Removed from SRT Source of Gstreamer\n");
}


/* This function will be called by the pad-added signal  and it will link demuxer with audio and video queue on recieving data*/
void GSTPipeline::pad_added_handler (GstElement *src, GstPad *new_pad, void  *data) {

  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));
  GSTPipeline *ptr= static_cast<GSTPipeline*>(data);
  if(g_str_has_prefix(GST_PAD_NAME(new_pad),"video"))
  {
  	GstPad *sink_pad = gst_element_get_static_pad (ptr->vqueue, "sink");
  	GstPadLinkReturn ret;
  	/* Attempt the link */
  	ret = gst_pad_link (new_pad, sink_pad);
  	if (GST_PAD_LINK_FAILED (ret)) {
    		g_print ("  Type is '%s' but link failed.\n", GST_PAD_NAME(new_pad));
  	} 
	else 
	{       g_print ("  CapsInfo (type '%s').\n",gst_caps_to_string (gst_pad_get_current_caps (new_pad)));
    		g_print ("  Link succeeded (type '%s').\n",GST_PAD_NAME(new_pad));

  	}

  }
  else if (g_str_has_prefix(GST_PAD_NAME(new_pad),"audio"))
  {
  	GstPad *sink_pad = gst_element_get_static_pad (ptr->aqueue, "sink");
  	GstPadLinkReturn ret;
  	/* Attempt the link */
  	ret = gst_pad_link (new_pad, sink_pad);
  	if (GST_PAD_LINK_FAILED (ret)) {
    		g_print ("  Type is '%s' but link failed.\n", GST_PAD_NAME(new_pad));
  	} 
	else 
	{
            g_print ("  CapsInfo (type '%s').\n",gst_caps_to_string (gst_pad_get_current_caps (new_pad)));
    		g_print ("  Link succeeded (type '%s').\n",GST_PAD_NAME(new_pad));

  	}
  }
  else
  {

    		g_print ("  Link not added succeeded (type '').\n" );

  }

}



// This needs to pass else conversion will not happen
bool GSTPipeline::gstINIT()
{
    /* GSTREAMER Initialisation */
    
    //gst_init(NULL, NULL);
    // It does not have any return so call another api to confirm

    GError* gsterror=NULL;
    if(!gst_init_check(NULL,NULL,&gsterror))
    {
        std::cout<<"Unable to Init: "<<gsterror->message<<"\n";
        g_error_free (gsterror);
        return false;
    }


    loop = g_main_loop_new(NULL, FALSE);
    if(!loop)
    {
        std::cout<<"Main Loop Creation failed\n";
        return false;
    }

    /* Create gstreamer elements */
    pipeline = gst_pipeline_new("srt-example");
    if (!GST_IS_PIPELINE (pipeline)) {
        return false;
    }

    srtsource = gst_element_factory_make("srtclientsrc", "srtsource");
    cqueue = gst_element_factory_make("queue", "cqueue");
    demuxer = gst_element_factory_make("tsdemux", "demuxer");
    aqueue = gst_element_factory_make("queue", "aqueue");
    vqueue = gst_element_factory_make("queue", "vqueue");
    asink = gst_element_factory_make("filesink", "asink");
    vsink = gst_element_factory_make("filesink", "vsink");


    if (!srtsource || !cqueue || !demuxer || !aqueue || !vqueue || !asink || !vsink) {
        std::cout<<"One element could not be created. Exiting.\n";
        return false;
    }
    return true;
} 

void GSTPipeline::setElementProperties(RecieveSRT *srtInfo)
{
    /* set properties */
    std::string srtin="srt://";
    srtin = srtin + srtInfo->getIPAdd() + ":" + srtInfo->getPortNo();
    //all are void type in return so always specify carefully
    g_object_set(G_OBJECT (srtsource), "uri", srtin.c_str(), NULL);
    g_object_set(G_OBJECT (srtsource), "mode",0, NULL);
    g_object_set(asink, "location", srtInfo->getAudioLoc().c_str(), NULL);
    g_object_set(vsink, "location", srtInfo->getVideoLoc().c_str(), NULL);
    g_object_set(G_OBJECT (demuxer),"emit-stats",gboolean (true),NULL);

}

int main(int argc,char** argv)
{
    RecieveSRT *srtInfo = new RecieveSRT();
    int count=0;
    while(count!=argc)
    {
        std::cout<<" "<<argv[count]<<std::endl;
        count++;
    }
    if(argc==5)
    {
        int count =0;
        srtInfo->setIPAddress(argv[count+1]);
        srtInfo->setPortNo(argv[count+2]);
        srtInfo->setVideoLoc(argv[count+3]);
        srtInfo->setAudioLoc(argv[count+4]);
    }
    else
    {
        std::cout<<" enter IP and port\n";
        std::cout<<"enter audio and video dump loc\n";
        delete srtInfo;
        return 0;
    }
    GSTPipeline *gstplayer= new GSTPipeline();
    if(gstplayer && gstplayer->gstINIT())
    {
        gstplayer->setElementProperties(srtInfo);
        gstplayer->addElementsBin();
        gstplayer->linkStaticElements();
        gstplayer->linkDynamicElements();
        gstplayer->setPipelineState(true);
        gstplayer->runMainEventLoop(true);
        delete gstplayer;
    }
    delete srtInfo;
    return 0;
}
