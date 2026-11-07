/* Native lift of DOS30a26/30644/305fc, with raster/modal/audio calls returned
 * explicitly to the frontend. All offsets refer to the original memory image. */
#include "message_dispatch.h"
#include <string.h>
#define DG 0x4d2c0u
static uint16_t rw(const uint8_t*m,unsigned a){return (uint16_t)(m[a]|m[a+1]<<8);}
static int r(const uint8_t*m,unsigned o){return (int16_t)rw(m,DG+o);}
static void ww(uint8_t*m,unsigned a,int n){m[a]=(uint8_t)n;m[a+1]=(uint8_t)((unsigned)n>>8);}
static void w(uint8_t*m,unsigned o,int n){ww(m,DG+o,n);}
static uint32_t rd(const uint8_t*m,unsigned o){return rw(m,DG+o)|((uint32_t)rw(m,DG+o+2)<<16);}
static void wd(uint8_t*m,unsigned o,uint32_t n){w(m,o,(int)n);w(m,o+2,(int)(n>>16));}
static unsigned fp(const uint8_t*m,unsigned o){return rw(m,DG+o)+16u*rw(m,DG+o+2);}
static int priority(const uint8_t*m,int id,int delta){unsigned a=16u*rw(m,DG+0x49f2)+(uint16_t)(id*6+0x60ac+delta);return (int16_t)rw(m,a);}
static int visible(const uint8_t*m){return m[DG+0x2969]&&(m[DG+0x2959]||m[DG+0x5326]==1);}
static unsigned stringlen(const uint8_t*m,unsigned a){unsigned n=0;while(a+n<1048576u&&m[a+n]&&n<32767u)++n;return n;}
static void text(SimMessageEffects*e,int x,int y,const char*s){unsigned n=0;SimMessageText*t;if(e->text_count==2)return;t=&e->text[e->text_count++];t->x=x;t->y=y;while(s[n]&&n<255u){t->text[n]=s[n];++n;}t->text[n]=0;}
static int due(uint8_t*m,unsigned o,int interval,uint32_t now){uint32_t old=rd(m,o);if((int32_t)now>=(int32_t)old&&(int32_t)now<(int32_t)(old+(uint32_t)interval))return 0;wd(m,o,now);return 1;}
static void dispatch(uint8_t*m,int notify,uint32_t now,SimMessageEffects*e,SimMessageEditorOpen hook,void*user){
 unsigned saved90=0,saved92=0;int restore_ca=0;
 int id=r(m,0x320c),n=r(m,0x4cce),p,x=r(m,0x298a),y=r(m,0x2988)+1;unsigned a,len;
 if(m[DG+0x32f2]||!id||!visible(m)||!(n<=r(m,0x4cd0)||n<r(m,0x4ccc)||r(m,0x3210)!=id))return;
 if(n){e->text_count=0;e->clear=1;e->x0=x;e->y0=y;e->x1=r(m,0x29be);e->y1=y+m[DG+0x120]-1;}
 w(m,0x4cce,0);if(r(m,0x3210)!=id)w(m,0x320e,r(m,0x3210));w(m,0x3210,id);
 if(id<0)m[DG+0x295a]=0;
 else{
  wd(m,0x4cd4,now);p=priority(m,id,0);
  if(p==6||p==8){
   m[DG+0xd1b3]=m[DG+0xc9da];m[DG+0xd1b2]=m[DG+0xc9d8];
   if(!m[DG+0x295d]){m[DG+0x295a]=1;e->icon=1;e->icon_x=x;e->icon_y=y;a=fp(m,0xc94e);if(a+1<1048576u)x+=(int16_t)rw(m,a);}
   else{
    int tx=m[DG+0xd1b3],ty=m[DG+0xd1b2],cols=r(m,0x296e),rows=r(m,0x296c),mx,my;
    saved90=rw(m,DG+0xca90);saved92=rw(m,DG+0xca92);restore_ca=1;w(m,0xca90,0);w(m,0xca92,0);
    if(m[DG+0x5326]!=1){e->open_editor=1;if(hook){w(m,0x320c,0);hook(m,1048576u,now,user);w(m,0x320c,id);}}
    m[DG+0xd1b3]=(uint8_t)(cols/2+(int8_t)m[DG+0x5f10]);m[DG+0xd1b2]=(uint8_t)(rows/2+(int8_t)m[DG+0x5f0e]);
    mx=tx-cols/2;my=ty-rows/2;if(mx<0)mx=0;if(my<0)my=0;if(mx>120-cols)mx=120-cols;if(my>100-rows)my=100-rows;
    m[DG+0x5f10]=(uint8_t)mx;m[DG+0x5f0e]=(uint8_t)my;e->viewport_changed=1;
   }
  }else m[DG+0x295a]=0;
  /* The coupled frontend executes the actual raster setter; legacy effects-only
   * callers retain their explicitly stubbed raster-state contract. */
  if(hook){w(m,0x124,0x0707);w(m,0x126,0x0808);}
  if((p==6||p==7)&&notify&&id!=32)e->alert3=1;
  w(m,0x4cd0,r(m,0x4cca)-(m[DG+0x295a]?2:0));w(m,0x3210,id);
  if(id<48){a=16u*rw(m,DG+0x49f2)+(uint16_t)(id*6+0x60a8);w(m,0x3208,rw(m,a));w(m,0x320a,rw(m,a+2));a=fp(m,0x3208);len=stringlen(m,a);w(m,0x4cce,(int)len);
   if((int)len>r(m,0x4cd0))w(m,0x4ccc,5-r(m,0x4cca));else text(e,x,y,(const char*)m+a);w(m,0x4cd2,x);
  }else{char s[24]="Message ";unsigned z=8,v=(unsigned)id;char b[6];unsigned k=0;do{b[k++]=(char)('0'+v%10);v/=10;}while(v&&k<6);while(k)s[z++]=b[--k];s[z]=0;text(e,x,y,s);}
 }
 if(restore_ca){w(m,0xca90,(int)saved90);w(m,0xca92,(int)saved92);}
 w(m,0x320c,0);
}
int sim_message_dispatch_with_editor(uint8_t*m,size_t size,int notify,uint32_t now,SimMessageEffects*e,SimMessageEditorOpen hook,void*user){if(!m||size<1048576u||!e){return 0;}memset(e,0,sizeof(*e));dispatch(m,notify,now,e,hook,user);return 1;}
int sim_message_dispatch(uint8_t*m,size_t size,int notify,uint32_t now,SimMessageEffects*e){return sim_message_dispatch_with_editor(m,size,notify,now,e,0,0);}
static void refresh(uint8_t*m,uint32_t now,SimMessageEffects*e,SimMessageEditorOpen hook,void*user){if(!visible(m))return;e->refresh_status=1;if(due(m,0x4cd4,r(m,0x31f2),now)||r(m,0x3210)==-1){w(m,0x3212,r(m,0x320c));w(m,0x31f2,126);dispatch(m,1,now,e,hook,user);}}
int sim_message_pump_with_editor(uint8_t*m,size_t size,uint32_t now,SimMessageEffects*e,SimMessageEditorOpen hook,void*user){
 int p=0,id,count,i,j,pos,len,cap;unsigned a;char line[256];
 if(!m||size<1048576u||!e){return 0;}memset(e,0,sizeof(*e));if(m[DG+0x32f2])return 1;
 pos=r(m,0x4ccc);len=r(m,0x4cce);cap=r(m,0x4cd0);
 if(due(m,0x4d08,pos<-4?1:4,now)&&visible(m)&&len>cap&&pos<=len&&cap>0&&cap<164){
  a=fp(m,0x3208);if(pos<0){if(pos<=-cap)pos=1-cap;w(m,0x4ccc,pos+1);for(i=0;i<cap;++i){j=i+pos+1;line[i]=j<0?' ':(a+(unsigned)j<1048576u?(char)m[a+(unsigned)j]:0);}line[cap]=0;}
  else{w(m,0x4ccc,pos+1);i=0;while(i<cap&&pos+i<len&&a+(unsigned)(pos+i)<1048576u){line[i]=(char)m[a+(unsigned)(pos+i)];++i;}if(len-pos<cap)line[i++]=' ';line[i]=0;}
  text(e,r(m,0x4cd2),r(m,0x29b8)+1,line);
 }
 id=r(m,0x4f74);if(id){w(m,0x4f74,0);
  if(id<0&&id!=-1){if(id>=-12&&id<=-10&&!due(m,0x4d0c,1080,now))return 1;if(id>-100){w(m,0x320c,-id);dispatch(m,1,now,e,hook,user);}e->modal_notification=id;e->modal_image_line=(int8_t)m[DG+0x2955];m[DG+0x2955]=255;if(id==-200)m[DG+0x2956]=1;return 1;}
  p=priority(m,id,0);count=r(m,0x3214);if(count<0||count>20)return 0;
  if(p<6||p>8){for(i=0;i<count;++i)if(r(m,0x4cd8+(unsigned)i*2)==id)goto inserted;}
  for(i=0;i<count&&p<=priority(m,r(m,0x4cd8+(unsigned)i*2),0);++i){}
  if(i!=20){if(count!=20)w(m,0x3214,++count);for(j=count-1;j>i;--j)w(m,0x4cd8+(unsigned)j*2,r(m,0x4cd8+(unsigned)(j-1)*2));w(m,0x4cd8+(unsigned)i*2,id);}
 }
inserted:
 if(r(m,0x320c)!=-1&&r(m,0x320c)!=0&&p<6)return 1;
 count=r(m,0x3214);if(count<0||count>20)return 0;if(!count){w(m,0x320c,-1);return 1;}
 id=r(m,0x4cd8);w(m,0x320c,id);for(i=0;i<count-1;++i)w(m,0x4cd8+(unsigned)i*2,r(m,0x4cda+(unsigned)i*2));w(m,0x3214,count-1);
 /* Original checks the following node's priority here, not this node's. */
 if(priority(m,id,6)>=6){w(m,0x31f2,0);refresh(m,now,e,hook,user);}return 1;
}
int sim_message_pump(uint8_t*m,size_t size,uint32_t now,SimMessageEffects*e){return sim_message_pump_with_editor(m,size,now,e,0,0);}
int sim_message_flush_with_editor(uint8_t*m,size_t size,uint32_t now,SimMessageEffects*e,SimMessageEditorOpen hook,void*user){if(!m||size<1048576u||!e){return 0;}memset(e,0,sizeof(*e));if(m[DG+0x32f2])w(m,0x4f74,0);else if(r(m,0x4f74))return sim_message_pump_with_editor(m,size,now,e,hook,user);return 1;}
int sim_message_flush(uint8_t*m,size_t size,uint32_t now,SimMessageEffects*e){return sim_message_flush_with_editor(m,size,now,e,0,0);}
int sim_messages_configure_mcga(uint8_t*m,size_t size){unsigned i,n,src,dst;if(!m||size<1048576u)return 0;for(i=0;i<64;++i){n=rw(m,DG+0x2b94+i*2);if(!n)break;src=fp(m,0x2dec+i*4);dst=fp(m,0x2b24+i*4);if(src+n>1048576u||dst+n>1048576u)return 0;memmove(m+dst,m+src,n);}if(i==64)return 0;m[DG+0x2950]=6;m[DG+0x108]=3;w(m,0xf4,0xa000);w(m,0xf6,320);w(m,0xf8,200);w(m,0x15ca,199);w(m,0x15cc,319);m[DG+0x120]=8;w(m,0x122,8);for(i=0;i<200;++i)w(m,0x13c+i*2,(int)i*320);src=fp(m,0x92f);if(src+104>1048576u)return 0;memmove(m+DG+0x51b0,m+src,104);w(m,0x4cca,22);return 1;}
int sim_messages_bind_icon(uint8_t*m,size_t size,const uint8_t*f,size_t n){if(!m||size<1048576u||!f||n<68||rw(f,0)!=8||rw(f,2)!=8)return 0;memmove(m+0x73000,f,68);w(m,0xc94e,0);w(m,0xc950,0x7300);return 1;}


const char *sim_message_modal_text(const uint8_t*m,size_t size,int id){unsigned i,a;if(!m||size<1048576u)return 0;for(i=0;i<21;++i){unsigned off=0x512c+i*6;if(!rw(m,DG+off)&&!rw(m,DG+off+2))break;if(r(m,off+4)==id){a=fp(m,off);return a<1048576u?(const char*)m+a:0;}}return 0;}
