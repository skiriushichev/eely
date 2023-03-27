# eely - Skeletal animation library

eely is a skeletal animation library written in C++.

It aims to provide engine-agnostic skeletal animation functionality as well as tools needed to import, view, modify, and debug animation-related data, such as skeletons, clips, animation graphs, etc.

It also serves as an educational project and a place where different implementations and features can be experimented with and benchmarked.

## Functionality

The current version includes:
* Importing skeletons and animation clips from FBX files
* Clip compression. This can be done either using [ACL](https://github.com/nfrechette/acl) library or with eely's fixed size compression that is optimized for forward playback
* Playing clips
* Layering animations with different masks
* Animation graphs. Graphs describe the logic of how a final pose is calculated and consist of nodes that perform different operations such as playing a single clip, combining clips, blending poses with different weights, running state machines, and others

Work in progress:
* Inverse kinematics with joint constraints

## Examples

All examples use Mixamo resources for importing animation clips and skeletons. For simplicity, all resources are imported and cooked when an application starts (this includes importing FBX files and building animation graphs). Another way would be to cook resources offline and then load eely's project file.

Examples that use animation graphs also can visualize them, to show their structure and runtime state.

### Playing clip

This example shows how to play a single clip imported from an FBX file.

<details>
<summary>GIF</summary>
<a href="https://github.com/skiriushichev/eely/blob/master/examples/00_clip/capture.gif">
<img src="https://github.com/skiriushichev/eely/blob/master/examples/00_clip/capture.gif" width=70%>
</a>
</details>

### Blending

This example shows how to blend multiple animations based on provided parameters.

There are 5 clips in total: walking, jogging, running, crouch walking and crouch running. Their blending is controlled by two parameters: movement speed [1.0 = walking, 2.0 = jogging, 3.0 = running] and crouching [0.0 = standing, 1.0 = crouching]. These parameters are controlled by the app and are fed into an animation graph.

This example also demonstrates phase synchronization between blended movement clips to avoid foot sliding.

<details>
<summary>GIF</summary>
<a href="https://github.com/skiriushichev/eely/blob/master/examples/01_blend/capture.gif">
<img src="https://github.com/skiriushichev/eely/blob/master/examples/01_blend/capture.gif" width=70%>
</a>
</details>

### Additive animations

This example shows how to apply additive animation on top of another pose.

There are 5 clips in total: walking, jogging, running, looking at -45 degrees (additive), and looking at +45 degrees (additive). The result pose is controlled by two parameters: movement speed [1.0 = walking, 2.0 = jogging, 3.0 = running] and look angle [in degrees, from -45.0 to +45.0].

Additive clips are precomputed and use skeleton masks to layer out every joint except the upper-body joints.

<details>
<summary>GIF</summary>
<a href="https://github.com/skiriushichev/eely/blob/master/examples/02_additive/capture.gif">
<img src="https://github.com/skiriushichev/eely/blob/master/examples/02_additive/capture.gif" width=70%>
</a>
</details>

### Simple state machine

This example shows how to make a simple state machine within an animation graph.

There are two states, each playing a single clip: idle and taunt. Taunt is played on request and then the state machine moves back to idle.

<details>
<summary>GIF</summary>
<a href="https://github.com/skiriushichev/eely/blob/master/examples/03_state_machine_simple/capture.gif">
<img src="https://github.com/skiriushichev/eely/blob/master/examples/03_state_machine_simple/capture.gif" width=70%>
</a>
</details>

### Complex state machine

This example shows a bigger state machine that includes nested state machines, more transitions with more complex conditions for them, as well as states that choose a random clip on each iteration.

There are 4 states in the state machine:
* Idle - single clip
* Blocking with a shield - this state runs a nested state machine underneath:
  * Begin blocking - raise a shield, single clip
  * Idle blocking - idle with a shield raised, single clip
* Slashing with a sword - there are two slashing animations, the state chooses one of them at random on each iteration
* Casting a spell - there are two casting animations, the state chooses one of them at random on each iteration

The animation graph is controlled by a single parameter: current character state. This parameter is then used by state transitions.

<details>
<summary>GIF</summary>
<a href="https://github.com/skiriushichev/eely/blob/master/examples/04_state_machine_complex/capture.gif">
<img src="https://github.com/skiriushichev/eely/blob/master/examples/04_state_machine_complex/capture.gif" width=70%>
</a>
</details>

### Inverse kinematics

This example app has two modes:
* IK mode - work in progress
* Constraints mode - joint limits are visualized and can be played with by rotating joints and seeing both constrained and unconstrained version

<details>
<summary>GIF</summary>
<a href="https://github.com/skiriushichev/eely/blob/master/examples/05_ik/capture.gif">
<img src="https://github.com/skiriushichev/eely/blob/master/examples/05_ik/capture.gif" width=70%>
</a>
</details>

## Building

eely requires CMake 3.23+.

Also, the importer and all examples depend on FBX SDK, which must be installed separately.

Right now, eely can only be built for Apple Silicon out of the box because almost all external dependencies (a complete list is in the `external` folder) are only precompiled for this platform. If you still want to build this project for another platform, currently the only option is to manually compile all dependencies and put binaries in `external` subfolders. 

Better support for other platforms will be added soon.

## License

See [LICENSE](https://github.com/skiriushichev/eely/blob/master/LICENSE)