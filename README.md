## BOEING

<p>A rewrite project of WebRTC network part </p>

### Description

<p>Usage: The algorithms are mainly for the transfer of udp reliable. Most of the parts are transformed from WebRTC.  Sometimes we use these algorithm to build up a reliable UDP system which is always be mentioned by SD-RTN or some other UDP based topics, especially using in media stream communacation </p>

### Compile

<p>Just execute make command from the root dir of the project</p>

### Notice

<p><mark>Just for study, no responsible for any bug or disaster that using online</mark></p>

### Topology
![Topology](https://raw.githubusercontent.com/ygliang2009/boeing/master/image/1.jpg)

### FEC
<p>The FEC module is to be added, the following diagram is mainly display the linking procedure when network change. This procedure is the following reaction of BitrateChangeCallback, which will trigger receiver update the frequency of sending FEEDBACK packet also. Allocating encoding rate is after FEC, will be the rest of FEEDBACK packet rate and FEC packet rate.</p>

![Topology](https://raw.githubusercontent.com/ygliang2009/boeing/master/image/2.jpg)
