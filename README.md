# USC EE450 Computer Networks Final Project

## Author

Karla Vasquez

## Overview

This project implements a distributed GitHub-like repository service using TCP and UDP sockets in C++. Users connect through a client as either a member or a guest. Member users can authenticate, look up repositories, push files, remove files, deploy their repository, and view an action log. Guest users can look up public repository contents only.

The system is split into one client, one main server, and three backend servers:

- `client.cpp` - connects to Server M over TCP, handles login, displays the command menu, sends user commands, and prints responses.
- `serverM.cpp` - main server. It accepts TCP connections from clients and coordinates requests with the backend servers over UDP.
- `serverA.cpp` - authentication server. It checks encrypted member credentials against `members.txt`.
- `serverR.cpp` - repository server. It manages file lookup, push, remove, deploy lookup data, and log history.
- `serverD.cpp` - deployment server. It receives deployment records and writes deployed files to `deployed.txt`.

## Features

- Member authentication using encrypted passwords.
- Guest access with lookup-only permissions.
- Repository lookup by username.
- File push with duplicate-file overwrite confirmation.
- File removal from a member repository.
- Repository deployment to Server D.
- Per-user command logging through the `log` command.

## Files

Source files:

- `client.cpp`
- `serverM.cpp`
- `serverA.cpp`
- `serverR.cpp`
- `serverD.cpp`
- `Makefile`

Data files:

- `original.txt` - plaintext member credentials for testing.
- `members.txt` - encrypted member credentials used by Server A.
- `filenames.txt` - initial repository contents.

Runtime-generated files:

- `deployed.txt` - created or appended to when a member runs `deploy`.
- `user_log.txt` - created or appended to when repository actions are logged.

## Ports

The project uses localhost (`127.0.0.1`) with the following ports:

- Server A UDP: `21985`
- Server R UDP: `22985`
- Server D UDP: `23985`
- Server M UDP: `24985`
- Server M TCP: `25985`

## Build

Compile all executables with:

```bash
make all
```

Clean generated executables with:

```bash
make clean
```

## Run

Open separate terminal windows or tabs from the project directory and start the servers:

```bash
./serverA
./serverR
./serverD
./serverM
```

Then start a member client using a username and plaintext password from `original.txt`:

```bash
./client <username> <password>
```

Example:

```bash
./client HannahWilliams598 pQQdZC2e2pjQ
```

To start a guest client:

```bash
./client guest guest
```

## Client Commands

Members can run:

```text
lookup <username>
lookup
push <filename>
remove <filename>
deploy
log
exit
```

Notes:

- `lookup <username>` returns the files associated with that user.
- `lookup` with no username defaults to the authenticated member's own repository.
- `push <filename>` adds a file to the member's repository.
- If a pushed filename already exists, the client asks whether to overwrite it.
- `remove <filename>` removes the file from the member's repository if it exists.
- `deploy` sends all files in the authenticated member's repository to Server D.
- `log` returns the authenticated member's repository action history.

Guests can run:

```text
lookup <username>
exit
```

## Message Flow

Client to Server M uses TCP. Server M communicates with backend servers over UDP:

- Authentication: Client -> Server M -> Server A -> Server M -> Client
- Lookup: Client -> Server M -> Server R -> Server M -> Client
- Push: Client -> Server M -> Server R -> Server M -> Client
- Push overwrite: Server R -> Server M -> Client -> Server M -> Server R
- Remove: Client -> Server M -> Server R -> Server M -> Client
- Deploy: Client -> Server M -> Server R, then Server M -> Server D
- Log: Client -> Server M -> Server R -> Server M -> Client

## Fixes and Current Status

The current version builds and runs on macOS/Linux-style C++ environments and includes fixes for the previous known issues:

- Socket `bind` calls now compile correctly on macOS.
- Guest login is handled explicitly by Server M.
- Member `lookup` without a username now looks up the authenticated user's repository.
- Push overwrite confirmation is forwarded correctly between Client, Server M, and Server R.
- Remove works immediately for newly pushed files.
- Deploy includes every repository file returned by Server R instead of skipping the first entry.
- The malformed repository row in `filenames.txt` was removed.

## References

Socket programming structure was adapted from:

- https://www.geeksforgeeks.org/socket-programming-in-cpp/
- https://beej.us/guide/bgnet/

The Makefile structure was adapted from:

- https://www.cs.swarthmore.edu/~newhall/unixhelp/howto_makefiles.html
