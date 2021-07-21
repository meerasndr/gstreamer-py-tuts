
## Foundations

### Elements

- basic building block of GStreamer pipelines
- typically an element has a very specific function, examples: read data from file, decode data, output data to sound card
- chain of elements linked together and data flows through this chain
- data: buffers and events
- chain of elements -> pipeline


### Pads

- pads are an element's input and output where you connect other elements
- pads negotiate links and data-flow
- caps negotiation: data type negotiation between pads of different elements
- examples: source pads, sink pads
