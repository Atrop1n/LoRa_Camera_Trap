**Adjustable parameters branch**

In this branch, I am trying to implement real-time bandwidth and spread factor adjustments via the webpage.

![image](https://github.com/user-attachments/assets/43cfc6f4-adad-44ce-8d2e-26a340fdaa2b)


If you configure LoRa bandwidth or spreading factor on the webpage, changes will immediately take effect on the receiver. Since both the sender and the receiver need to be
configured with the same bandwidth and SF, sender will stop receiving acknowledgment packets for a while, since reveicer is now using different parameters. The sender will
sense this, and after not receiving any acknowldegment packet for a specified period (_ack_wait_timeout_) it will now periodically switch to various combinations of
BW and SF, listening on the same variation for a period specified by _SF_find_timeout_. When an _ack_ packet is received at a particular combination of BW and SF during this time window, the sender keeps these parameters and transmission continues normally.

Now this part is tricky. If _SF_find_timeout_ is too short, no packet will be received because the time would be insufficient for a packed to be parsed. This is particularly obvious on low bandwidths and high spread factors. 
If _SF_find_timeout_ is too long, it may take up to several minutes for a sender to finally find the correct combination of BW and SF. 
Tweak _SF_find_timeout_ so that it suits your particular use scenario. At SF = 7 and BW = 500 kHz 500 ms is usually enough, but SF = 12 and BW = 7,8 kHz would need up to 30 seconds. 

**If you find a reliable way to ensure the sender is notified when receiver's parameters have changed, and adjusts own parameters accordingly, you are free to contribute!**
