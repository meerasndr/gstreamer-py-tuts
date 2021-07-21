
1. **Clean core/plugins separation**
<code>
The core of GStreamer is essentially media-agnostic. It only knows about bytes and blocks, and only <br> contains basic elements. The core of GStreamer is even functional enough to implement <br> low-level system tools, like cp
</code>
What does this exactly mean?

2. **Queries**
Queries allow applications to request information such as duration or current playback position from the pipeline. *Queries are always answered synchronously.*
How? Why?

3. **Keyboard Interrupts in Python**
Pressing Ctrl+C when the pipeline is in playing state immediately ends playback in C. How do we achieve this in Python? Possibly using try-catch?
