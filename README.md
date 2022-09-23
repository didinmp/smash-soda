<!-- PROJECT LOGO -->
<br />
<p align="center">
  <img src="https://github.com/MickeyUK/SmashSoda/raw/master/github/logo.png">
  <h3 align="center">Smash Soda</h3>

  <p align="center">
    Open source Parsec Arcade hosting tool
    <br />
    <a href="https://github.com/MickeyUK/SmashSoda/releases">Latest Release</a>
    ·
    <a href="https://github.com/MickeyUK/SmashSoda/issues">Report Bug</a>
    ·
    <a href="https://github.com/MickeyUK/SmashSoda/issues">Request Feature</a>
  </p>
</p>

<!-- TABLE OF CONTENTS -->
<details open="open">
  <summary><h2 style="display: inline-block">Table of Contents</h2></summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
    </li>
    <li><a href="#features">Features</a></li>
    <li><a href="#chat-commands">Chat Commands</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgements">Acknowledgements</a></li>
  </ol>
</details>


## About the Project

This is a modification of <a href="https://github.com/FlavioFS/">ParsecSoda</a>, a tool developed by <a href="https://github.com/FlavioFS/">FlavioFS</a> for improving the hosting experience on Parsec Arcade. It builds upon modifications made by user <a href="https://github.com/v6ooo/">v6000</a>.

The aim of this project is to add additional useful features, with a focus on those who wish to host Super Smash Bros Ultimate.

This is an experimental version of Parsec Soda...expect bugs! Help development by reporting <a href="https://github.com/MickeyUK/SmashSoda/issues">issues</a> and contributing to the <a href="https://github.com/MickeyUK/SmashSoda/discussions">Discussions</a>!

## <a href="https://github.com/MickeyUK/SmashSoda/releases">DOWNLOAD LATEST VERSION HERE</a>

## Features

  * **Smash Bros Ultimate**

  Some new admin chat commands have been added for making hosting Smash easier. **!bb** will press everybody's B button twice...good when you're stuck the the match results screen!

  Doing 2v2 tournaments on Parsec is tricky, what with the ingame tournament mode not having an option for it. Some new admin chat commands have been added to make it much easier. 
  
  **!teams random** will automatically sort all the guests in your room in to teams (or aborting if uneven guest count). Admins can then use the command **!teamswin 1/2** to tell Soda which team has won and it will automatically keep track of everybody's position. It will also automatically put the guests in to the gamepad slots when it is there turn! See all the chat commands <a href="#teams">here</a>.

  * **Ping Limit**

  This allows you to set a latency limit for your lobby. Those whose pings exceed the limit will be kicked automatically.

  * **Moderator System**

  Grant moderator permissions to users you trust in your lobbies. This will allow your mods to kick troublesome users and other admin tier commands.

  * **Hotseat**

  Hosting a single player game and people aren't sharing nicely? Well now you can set a hotseat timer...when the timer is up, it'll go down the guest list and swap them in to the gamepad 1 slot!

  * **Themes**
  
  Check out the new *Midnight* theme for Smash Soda or select from some presets!

## Chat Commands

Below is a list of all the chat commands.

##### General

Standard guests, mods and the host can use these commands.

|Command|Description|
| ----------- | ----------- |
|!bonk| Bonk another user on the head.
|<b>!discord</b>| If you have set a Discord invite URL set in the Smash Soda configuration settings then this command will print it in chat.
|!help| Show command list.
|!ff| Drop all gamepads you're holding.
|!mirror| Toggle mirroring of L-Stick into DPad.
|!one| Maps all of your devices to the same gamepad.
|!pads| Shows who's holding each gamepad.
|!sfx| Play sound effect.
|<b>!spectate</b>| Be skipped by hotseat and team commands
|!swap| Replace your gamepad with another one.

##### Admin / Moderator

Admins, mods and the host can use these commands.

|Command|Description|
| ----------- | ----------- |
|!ban| Ban a guest.
|<b>!bb</b>| Press everybody's B button twice.
|!dc| Disconnect a specific gamepad.
|!kick| Kick user from the room.
|!limit| Set the maximum amount of pads a guest can hold.
|!strip| Strip gamepad from player's hand.
|<b>!ping</b>| Returns the ping for a user.
|!unban| Unban a guest.

##### God

Only the host can use these commands.

|Command|Description|
| ----------- | ----------- |
|!gameid| Set game ID.
|!guests| Set the amount of room slots.
|!mic| Set the microphone volume.
|<b>!mod</b>| You can make a user in your lobby a moderator. They will then have Admin tier privileges, having access to those commands.
|!name| Set the room name.
|!private| Make the room private.
|!public| Make the room public.
|!q| Close stream.
|!setconfig| Apply config changes.
|!speakers| Set speakers volume.
|<b>!unmod</b>| Revoke moderator permissions.

## Contributing

See the [open issues](https://github.com/MickeyUK/SmashSoda/issues) for a list of proposed features (and known issues).

Would you like to contribute to the project? That's great! Here's what you do:


1. Open a new issue reporting what you're going to do.
2. Fork this repository.
3. Create a branch for your feature.
4. Make your local changes.
5. Submit a pull request.

I am working on this project as a hobby and an opportunity for me to learn more about working with C++. If you love Smash Soda, consider <a href="https://www.paypal.com/donate?hosted_button_id=28PBV9DFYQC72">donating to FlavioFS</a> for his amazing work on Parsec Soda!

Hey though, if you'd like to <a href="https://ko-fi.com/mickeyuk">buy me a coffee</a>, I won't say no!

## License

See `LICENSE.txt` for more information.


## Contact


Project Link: [https://github.com/MickeyUK/SmashSoda](https://github.com/MickeyUK/SmashSoda)



<!-- ACKNOWLEDGEMENTS -->
## Acknowledgements

* [MickeyUK] - [GitHub](https://github.com/MickeyUK)
* [FlavioFS] - [GitHub](https://github.com/FlavioFS/)
* [v6000] - [GitHub](https://github.com/v6ooo/)
* [R3DPanda] - [GitHub](https://github.com/R3DPanda1/)