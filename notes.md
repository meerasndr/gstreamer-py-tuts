
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

### Communication
**Downstream** - data flow from sources to sinks
**Upstream** - data flow from sinks to sources

#### Buffers
- objects
- pass streaming data between elements in the pipeline
- always travel downstream

#### Events
- objects
- sent between elements or from application to elements
- can travel upstream and downstream

#### Messages
- objects
- posted by elements on the pipeline's message bus
- used to transmit info like: errors, state changes, tags, buffering state(?), redirects
- usually handled asynchronously by the application in a seprate thread

#### Queries
- objects(?)
- applications use queries to request info 
