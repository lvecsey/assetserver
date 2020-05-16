
You can use assetserver to deliver files over TCP, for example to an Android application.

Run an assetserver:

```console
    tcpserver -vRHl0 192.168.2.14 5980 ./assetserver /mnt/export
```

Then fetch a file:

```console
     VERBOSE=1 tcpclient -vRHl0 192.168.2.14 5980 ./gfas images/samplefile.png
```

The above retrieves the file from /mnt/export/images/samplefile.png and saves
it to the current directory as samplefile.png

In an actual app you could initiate a TCP connection, and run the assetserver protocol to keep the file in memory.



