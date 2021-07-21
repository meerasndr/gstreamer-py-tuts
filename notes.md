
## Foundations

### Elements

- basic building block of GStreamer pipelines
- typically an element has a very specific function, examples: read data from file, decode data, output data to sound card
- chain of elements linked together and data flows through this chain
- **data: buffers and events**
- chain of elements -> pipeline


### Pads

- pads are an element's input and output where you connect other elements
- pads negotiate links and data-flow
- caps negotiation: data type negotiation between pads of different elements
- examples: source pads, sink pads


### Bins
- collection of elements
- bins are also just sublcasses of elements
- Example: change state of bin => changes state of all elements in said bin

### Pipelines
- top-level bin
- all pipelines come with a bus -> applications and pipeline are able to communicate throug the bus
- manage synchronization for its children
- Pipeline states: NULL, READY, PAUSED, PLAYING
- pipelines run in a seprate thread, until stopped or EOS is reached
