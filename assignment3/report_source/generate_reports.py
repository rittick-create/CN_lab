from pathlib import Path
import csv,collections,statistics
from reportlab.platypus import SimpleDocTemplate,Paragraph,Spacer,Table,TableStyle,PageBreak,Flowable,KeepTogether
from reportlab.lib.styles import getSampleStyleSheet,ParagraphStyle
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.pdfgen import canvas
from reportlab.lib.enums import TA_CENTER
from xml.sax.saxutils import escape
ROOT=Path('/Users/rittickdutta/Desktop/CN_lab'); W=A4[0]-88
NAVY=colors.HexColor('#17324d'); BLUE=colors.HexColor('#26678c'); GRAY=colors.HexColor('#526170')
styles=getSampleStyleSheet()
for name,kw in [('T',dict(fontName='Helvetica-Bold',fontSize=23,leading=28,textColor=NAVY,spaceAfter=12)),('S',dict(fontSize=13,leading=18,textColor=GRAY,spaceAfter=16)),('H',dict(fontName='Helvetica-Bold',fontSize=15,leading=20,textColor=NAVY,spaceAfter=10)),('h',dict(fontName='Helvetica-Bold',fontSize=11.5,leading=16,textColor=NAVY,spaceBefore=8,spaceAfter=5)),('B',dict(fontSize=10.1,leading=14.2,spaceAfter=8)),('Sm',dict(fontSize=8.7,leading=11.8,spaceAfter=7)),('C',dict(fontName='Courier',fontSize=8.6,leading=12,backColor=colors.HexColor('#f0f4f7'),borderPadding=8,spaceAfter=12)),('Cell',dict(fontSize=8.6,leading=11.5)),('Head',dict(fontName='Helvetica-Bold',fontSize=8.6,leading=11.5,textColor=colors.white))]:styles.add(ParagraphStyle(name=name,**kw))
class HeaderCanvas(canvas.Canvas):
 def __init__(self,*a,**k):super().__init__(*a,**k);self.states=[]
 def showPage(self):self.states.append(dict(self.__dict__));self._startPage()
 def save(self):
  n=len(self.states)
  for st in self.states:
   self.__dict__.update(st);self.setStrokeColor(colors.HexColor('#d2dce4'));self.line(44,38,A4[0]-44,38);self.setFont('Helvetica',8);self.setFillColor(GRAY);self.drawString(44,25,'Rittick Dutta | 002410501088 | BCSE-3, A3');self.drawRightString(A4[0]-44,25,f'{self._pageNumber} / {n}');self.drawString(44,A4[1]-27,'Computer Networks Lab');self.drawRightString(A4[0]-44,A4[1]-27,self._cn_label);super().showPage()
  super().save()
class Flow(Flowable):
 def __init__(self,labels):super().__init__();self.labels=labels;self.width=W;self.height=75
 def draw(self):
  c=self.canv;n=len(self.labels);bw=(W-12*(n-1))/n
  for i,s in enumerate(self.labels):
   x=i*(bw+12);c.setFillColor(colors.HexColor('#eef4f8'));c.setStrokeColor(BLUE);c.roundRect(x,20,bw,40,5,stroke=1,fill=1);c.setFillColor(NAVY);c.setFont('Helvetica-Bold',8);c.drawCentredString(x+bw/2,43,s)
   if i<n-1:c.setStrokeColor(BLUE);c.line(x+bw,40,x+bw+10,40);c.line(x+bw+10,40,x+bw+7,43);c.line(x+bw+10,40,x+bw+7,37)
class Graph(Flowable):
 def __init__(self,title,labels,series,ymax=None):super().__init__();self.title=title;self.labels=labels;self.series=series;self.width=W;self.height=155;self.ymax=ymax or max(max(v) for _,v,_ in series)*1.12
 def draw(self):
  c=self.canv; x0,y0=40,35;pw=W-55;ph=95;c.setFillColor(NAVY);c.setFont('Helvetica-Bold',9.5);c.drawString(0,145,self.title)
  for t in range(5):
   y=y0+t*ph/4;c.setStrokeColor(colors.HexColor('#dce4e9'));c.line(x0,y,x0+pw,y);c.setFont('Helvetica',7);c.setFillColor(GRAY);c.drawRightString(x0-5,y-2,f'{self.ymax*t/4:.2g}')
  for i,l in enumerate(self.labels):
   x=x0+(i+.5)*pw/len(self.labels);c.setFont('Helvetica',7.3);c.setFillColor(GRAY);c.drawCentredString(x,19,str(l))
  for name,vals,col in self.series:
   c.setStrokeColor(col);c.setFillColor(col);c.setLineWidth(1.7)
   pts=[(x0+(i+.5)*pw/len(vals),y0+v/self.ymax*ph) for i,v in enumerate(vals)]
   for a,b in zip(pts,pts[1:]):c.line(*a,*b)
   for x,y in pts:c.circle(x,y,2.4,fill=1,stroke=0)
  x=42
  for name,vals,col in self.series:
   c.setFillColor(col);c.circle(x,5,3,fill=1,stroke=0);c.setFillColor(GRAY);c.setFont('Helvetica',7.3);c.drawString(x+6,2,name);x+=max(70,len(name)*5+20)
def builder(num,subtitle):
 story=[]
 def p(s,st='B'):story.append(Paragraph(s,styles[st]))
 def h(s):p(s,'H')
 def sub(s):p(s,'h')
 def code(s):p(escape(s).replace('\n','<br/>'),'C')
 def table(headers,rows,widths=None):
  data=[[Paragraph(escape(str(v)),styles['Head']) for v in headers]]+[[Paragraph(str(v),styles['Cell']) for v in row] for row in rows]
  t=Table(data,colWidths=widths or [W/len(headers)]*len(headers),repeatRows=1,hAlign='LEFT');t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),NAVY),('VALIGN',(0,0),(-1,-1),'TOP'),('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.HexColor('#f1f5f8'),colors.white]),('BOTTOMPADDING',(0,0),(-1,-1),6),('TOPPADDING',(0,0),(-1,-1),6),('LINEBELOW',(0,-1),(-1,-1),.5,colors.HexColor('#cad5dd'))]));story.extend([t,Spacer(1,10)])
 def page():story.append(PageBreak())
 p('COMPUTER NETWORKS LABORATORY','Sm');p(f'Assignment {num}','T');p(subtitle,'S')
 return story,p,h,sub,code,table,page
# Assignment 3
story,p,h,sub,code,table,page=builder(3,'CSMA techniques with collision detection')
table(['Student information','Details'],[['Name','Rittick Dutta'],['Roll number','002410501088'],['Class / group','BCSE-3 / A3'],['Course','CSE/PC/B/S/314 - Computer Networks Lab'],['Report date','02 November 2026'],['Code demonstration deadline','21-25 September 2026'],['Report deadline','Next lab class after demonstration']],[165,W-165])
sub('Problem statement');p('Implement Non-Persistent, 1-Persistent, p-Persistent and CSMA/CD on a shared channel. Independent station processes sense the channel, transmit frames, detect or infer collisions, and back off. Compare collision count, mean delay and throughput against p and station count N.')
sub('Design and purpose');p('Each station is an independent process connected through a localhost TCP socket to the receiver/channel process. The receiver advances a slotted shared-channel state (IDLE, BUSY or COLLISION), gathers simultaneous requests and reports success or failure. TCP carries the simulation messages; slot timing and collision decisions are modelled in application code.')
story.append(Flow(['Station processes','Carrier sensing','Shared channel','Delivery + metrics']))
sub('Input and output');p('<b>Inputs:</b> N, scheme (non, one, p, cd), frames/station, p and seed; manual mode also accepts a destination and text message. <b>Outputs:</b> delivered frames, console metrics, CSV measurements and SVG plots in output/.')
page();h('Implementation and test cases')
code('cd /Users/rittickdutta/Desktop/CN_lab/assignment3\nmake\n./evaluate_app --sweep\n./evaluate_app 4 cd')
p('CSMA.cpp defines carrier sensing, channel state, scheme selection, collision handling and backoff. sender.cpp launches a station; receiver.cpp runs the shared channel; evaluate.cpp forks station processes, records metrics and writes comparison plots.')
table(['Scheme','Idle/busy action','Collision response'],[['Non-Persistent','On busy, wait random 1-8 slots before sensing again.','Random 0-7 slot backoff.'],['1-Persistent','Sense every slot; send immediately when idle.','Random 0-7 slot backoff.'],['p-Persistent','On idle, send with probability p; otherwise wait one slot.','Random 0-7 slot backoff.'],['CSMA/CD','1-persistent sensing; abort a detected collision early.','Binary exponential backoff.']],[110,210,W-320])
p('A frame normally lasts 4 slots; one-way propagation is 1 slot. In this model, CSMA/CD detects a worst-case collision when transmission time is at least twice propagation time. If shorter, a failed frame yields NO_ACK at completion and is retried. This is a slotted model, not a physical Ethernet signal emulator.')
sub('Verification cases');table(['Case / input','Expected check','Observed'],[['Build: make','All three executables compile','Passed after correcting stray = in CSMA.cpp.'],['Default: ./evaluate_app 4 cd','20 frames eventually delivered','Evaluated; metrics saved in CSV.'],['p sweep: 0.1 to 1, N=4','All 15 runs yield ordered delivery','Passed; 3 seeds per p.'],['N sweep: 2,4,8,12','All 48 runs yield ordered delivery','Passed; 3 seeds per scheme/N.'],['Short frame: Tx=1, prop=1','NO_ACK rather than early detection','Model branch provided in source.'],['Manual addressed message','Successful frame reaches destination','Receiver forwards accepted frames.']],[150,180,W-330])
p('The recorded sweep contains 63 runs: 5 p values x 3 seeds, then 4 schemes x 4 station counts x 3 seeds. Each station sends five frames. The last two manual branches are described from the implementation; the saved sweep is the source of plotted measurements.','Sm')
# group data
P=list(csv.DictReader(open(ROOT/'assignment3/output/p_results.csv'))); N=list(csv.DictReader(open(ROOT/'assignment3/output/N_results.csv')))
def avg(rows,k):return statistics.mean(float(r[k]) for r in rows)
ps=[.1,.25,.5,.75,1.0]; ns=[2,4,8,12]; schemes=[('non','Non-Persistent',colors.HexColor('#26678c')),('one','1-Persistent',colors.HexColor('#c26a3b')),('p','p-Persistent',colors.HexColor('#51936a')),('cd','CSMA/CD',colors.HexColor('#8563aa'))]
page();h('Results - p-Persistent CSMA at N = 4');p('Each point is the arithmetic mean of three seeded runs. The best tested p is 0.25 by mean throughput for this workload.')
table(['p','Collisions','Mean delay (slots)','Throughput (frames/slot)'],[[f'{v:g}',f'{avg([r for r in P if float(r["p"])==v],"collisions"):.2f}',f'{avg([r for r in P if float(r["p"])==v],"delay"):.2f}',f'{avg([r for r in P if float(r["p"])==v],"throughput"):.3f}'] for v in ps],[70,100,140,W-310])
for k,title in [('collisions','Collision events versus p'),('delay','Mean frame delay (slots) versus p'),('throughput','Throughput (frames/slot) versus p')]:story.append(Graph(title,[f'{v:g}' for v in ps],[('p-Persistent',[avg([r for r in P if float(r['p'])==v],k) for v in ps],BLUE)]))
page();h('Results - effect of station count N');p('For each scheme and N, values are means of three runs with five frames per station. p-Persistent uses the selected p = 0.25.')
for k,title in [('collisions','Collision events versus N'),('delay','Mean frame delay (slots) versus N'),('throughput','Throughput (frames/slot) versus N')]:story.append(Graph(title,ns,[(label,[avg([r for r in N if r['scheme']==s and int(r['N'])==v],k) for v in ns],col) for s,label,col in schemes]))
page();h('Analysis and comments')
sub('Metrics and observations');p('Collision count is the number of shared collision events. Delay is mean slots from frame readiness until reception, including waiting and retries. Throughput is delivered frames divided by elapsed slots. Efficiency equals throughput x 4 (the frame duration); ideal throughput is 0.25 frames/slot in this model.')
table(['N','Non-Persistent','1-Persistent','p-Persistent','CSMA/CD'],[[str(v)]+[f'{avg([r for r in N if r["scheme"]==s and int(r["N"])==v],"throughput"):.3f}' for s,_,_ in schemes] for v in ns],[55,115,110,115,W-395])
p('At N=4, p=0.25 reaches about 0.159 frames/slot. p=1 causes 32 collisions per run on average and reduces throughput to 0.090. At N=12, 1-persistent CSMA falls to about 0.004 frames/slot, as simultaneous aggressive retries create heavy contention. Non-persistent waiting avoids many repeated collisions. CSMA/CD reduces collision occupancy when the timing condition permits detection, although backoff can leave idle slots.')
sub('Correctness, limits and possible improvements');p('The sweep completed with ordered, exactly-once delivery in the generated data. Results depend on finite batches and random seeds; p=0.25 is the best among tested values, not a universal optimum. TCP loopback is only the control/data carrier for the emulation; the measured slot throughput is not TCP throughput. More seeds, longer runs and a finer p grid would improve confidence. Physical propagation, jam bits and capture of actual collisions are outside this model.')
sub('Comments on the assignment');p('The exercise demonstrates why a station must balance prompt transmission against collision risk. Implementing four methods behind one sender/channel interface makes their performance differences visible. A useful extension would specify an exact propagation model and require repeated-run confidence intervals.')
out=ROOT/'assignment3/output/pdf/Assignment3_Rittick_Dutta_CSMA_Report.pdf';HeaderCanvas._cn_label='Assignment 3 | CSMA';SimpleDocTemplate(str(out),pagesize=A4,leftMargin=44,rightMargin=44,topMargin=48,bottomMargin=50,title='Assignment 3 - CSMA',author='Rittick Dutta').build(story,canvasmaker=HeaderCanvas);print(out)
# Assignment4
story,p,h,sub,code,table,page=builder(4,'CDMA with Walsh codes')
table(['Student information','Details'],[['Name','Rittick Dutta'],['Roll number','002410501088'],['Class / group','BCSE-3 / A3'],['Course','CSE/PC/B/S/314 - Computer Networks Lab'],['Report date','02 November 2026'],['Code deadline','05-09 October 2026'],['Report deadline','Next lab class after code demonstration']],[165,W-165])
sub('Problem statement');p('Implement code-division multiple access for n stations. Assign each sender a unique Walsh code, combine their encoded data on one shared medium, and reconstruct every station\'s data at the receivers.')
sub('Design and purpose');p('Walsh code length L is the smallest power of two at least n. Orthogonal rows let every station share the same chip vector. A data bit 1 maps to +1, bit 0 to -1, and a silent station to 0. The transmitter adds all code-scaled vectors; each receiver computes a dot product with its own code and divides by L.')
story.append(Flow(['n sender files','Walsh code set','Sum encoded chips','Correlate + write']))
sub('Input and output');p('The program prompts for n and reads sender/sender1.txt through sender/sendern.txt as binary files. It creates receiver/ and writes matching receiver/sender*.txt files. Different file lengths are supported: a station becomes silent after its input ends.')
page();h('Implementation and worked example')
code('cd /Users/rittickdutta/Desktop/CN_lab/assignment4\n/usr/bin/clang++ -std=c++17 main.cpp walsh.cpp transmission.cpp receiver.cpp -o cdma\nprintf "4\\n" | ./cdma\nfor f in sender/*.txt; do cmp "$f" "receiver/${f#sender/}"; done')
p('cdma.h declares the modular interfaces walsh(n), transmit(data,codes), receive(signal,codes). walsh.cpp builds Sylvester Walsh rows recursively; transmission.cpp sums each data value multiplied by its code; receiver.cpp computes each normalized correlation. main.cpp streams one byte per active station, processes bits most-significant first, and writes recovered bytes.')
sub('Four-station Walsh set');table(['Station','Codeword'],[['1','[+1,+1,+1,+1]'],['2','[+1,-1,+1,-1]'],['3','[+1,+1,-1,-1]'],['4','[+1,-1,-1,+1]']],[90,W-90])
p('For one symbol interval let data be [+1,-1,0,+1]. The combined chip vector is C1 - C2 + C4 = <b>[+1,+1,-1,+3]</b>. Correlation with each code then gives:')
table(['Receiver','Dot product / 4','Recovered value'],[['1','4 / 4','+1'],['2','-4 / 4','-1'],['3','0 / 4','0'],['4','4 / 4','+1']],[100,170,W-270])
p('Because distinct Walsh rows have dot product zero and each row has squared length L, the sum separates exactly in this noiseless integer model.')
page();h('Test cases and results')
table(['Input / case','What it checks','Observed result'],[['n=4; four supplied files','Unequal lengths and empty sender4','22, 59, 120, 0 bytes recovered exactly.'],['n=1','Smallest Walsh set L=1','Single-row code [+1] supported.'],['n=3','Non-power-of-two station count','Three rows of four chips.'],['Data values -1,0,+1','Bit zero, silence and bit one','Orthogonal correlation recovers each value.'],['Missing sender file','Input error handling','Program emits an error and exits nonzero.'],['Bad n / mismatched lengths','Validation in functions','Invalid input rejected.']],[145,170,W-315])
p('The four-file run was executed from assignment4. Each sender file was compared with its corresponding receiver file using cmp; all comparisons passed. The input sizes total 201 bytes, or 1,608 source data bits. The longest file has 120 bytes, so the parallel run has 960 bit intervals; with L=4, the model produces 3,840 shared-channel chip values. These chips are transient vectors and are not saved to a file.')
code('sender1.txt:  22 bytes  -> receiver/sender1.txt:  22 bytes\nsender2.txt:  59 bytes  -> receiver/sender2.txt:  59 bytes\nsender3.txt: 120 bytes  -> receiver/sender3.txt: 120 bytes\nsender4.txt:   0 bytes  -> receiver/sender4.txt:   0 bytes')
sub('Verification rule');p('For every sender i, byte-for-byte equality is the success condition. The empty file checks that a station may remain silent. Unequal lengths check that a finished station stays silent while longer senders continue.')
page();h('Analysis and comments')
sub('Correctness and limits');p('Every station shares one vector at each bit interval, while its Walsh row identifies its contribution. Dividing the correlation by code length, rather than by station count, gives the original symbol. At n=4, L=4; at n=3, L is still 4 because Walsh size is a power of two. This implementation assumes exact chip alignment, no channel noise, and integer arithmetic. It emulates a CDMA medium in memory; it does not send radio signals or network packets.')
sub('Known constraints and possible improvements');p('The program expects sender1.txt through sendern.txt to exist. It uses one code per station and processes bytes MSB first. The receiver uses integer division, which is exact for the orthogonal noiseless signals produced by the transmitter; a noisy signal would need thresholds or error handling. Further work could add noisy chips, timing offsets, measured bit-error rates and explicit chip-output logging.')
sub('Comments on the assignment');p('The experiment makes orthogonality practical: independent users can transmit concurrently because correlation isolates each code. The file-based sender/receiver interface also shows that silence must be represented separately from a zero bit. A small worked chip example helps clarify this distinction.')
out=ROOT/'assignment4/output/pdf/Assignment4_Rittick_Dutta_CDMA_Report.pdf';HeaderCanvas._cn_label='Assignment 4 | CDMA';SimpleDocTemplate(str(out),pagesize=A4,leftMargin=44,rightMargin=44,topMargin=48,bottomMargin=50,title='Assignment 4 - CDMA',author='Rittick Dutta').build(story,canvasmaker=HeaderCanvas);print(out)
