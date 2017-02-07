
The Curve
======

Idea
-----

Every Value changes over time, but can be described in a sequence of changes. 
At every moment in time, the value can be calculated based on the interpolation method used for it, and the current state.
The sequence of changes can be calculated the moment a unit receives a movement command, and this sequence is not constrained. A prediction about a movement can be made, and at any point in the future evaluated, without the need to recalculate.

This is a huge benefit for the networking - since only actions and chances to those actions are transmitted, but not as commands but as consequence of these commands. 

Types
------
The minimal set of curves is for the following types of values, and possible combinations: 

 * **SimpleContinuous** A single dimensional value, has to support any assignment and mathematical operation. It can be interpreted as continuous value and will be interpolated linear between two points in time.
 * **SimpleDiscrete** A single dimensional value, only supporting discrete steps, issued as starting point and value. An interpolation is done by finding the closest value.
  * **Multidimensional** A multidimensional value with a constant dimension
  * **Event** An Event that happens at _exactly_ one point in time, that should trigger certains things within the renderer/client, like explosions.
  * **EventList** Multiple events to be filtered within a [from-to) window
  * **Object** A freely-typed object, that has to be sub-typed for arbitrary types to be traced by curves.
  * **List** Contains Objects, but also keeps track of their creation/destruction based on a timeline

Networking Features
-----
The Networkign has to only transmit keyframes, and no other interpolation/state information.

The local state of each client is tracked by the server, and changes in the curve model on the server trigger updates to be sent to the client. 
There are two ways of handling a keyframe update: 

1. Append new keyframes
2. Insert keyframes inbetween
3. Insert new keyframes and remove everything that was there after the first newly inserted timestamp (replace the whole future)
4. Change existing keyframes (update only their values)
5. Selectively erase existing keyframes

Here are some examples of scenarios where these interpolating features are relevant

 * Factory creating a new unit object (append)
 * User giving a unit a command (insert & remove)
 * Removing from the production queue (remove)
 * Killign a unit (remove)
 * Shooting an arrow (insert inbetween)


