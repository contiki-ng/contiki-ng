/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         A Java IPv4 transport for CoAP + LWM2M
 *         Converts hex input to UDP packets and incoming packets to hex out.
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */
import javax.xml.bind.DatatypeConverter;
import java.net.DatagramSocket;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.io.IOException;
import java.util.Arrays;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.io.PrintStream;

public class Hex2UDP implements Runnable {

    DatagramSocket serverSocket;
    InetAddress address;
    int port;

    Hex2UDP(String host, int port) {
        try {
            serverSocket = new DatagramSocket();
            this.address = InetAddress.getByName(host);
            this.port = port;
            new Thread(this).start();
        } catch(Exception e) {
            /* Do good stuff here... */
            e.printStackTrace();
        }
    }

    public void run() {
        byte[] receiveData = new byte[1024];
        /* The receive loop */
        while(true) {
            DatagramPacket receivePacket =
                new DatagramPacket(receiveData, receiveData.length);
            try {
                /* Receive a packet */
                serverSocket.receive(receivePacket);
            } catch (IOException e) {
                e.printStackTrace();
            }
            byte[] data2 = Arrays.copyOf(receivePacket.getData(),
                                         receivePacket.getLength());
            receive(data2);
        }
    }

    public void send(byte[] data) throws IOException {
        DatagramPacket sendPacket =
            new DatagramPacket(data, data.length, address, port);
        serverSocket.send(sendPacket);
    }

    /* Override this to make something more sensible with the data */
    public void receive(byte[] data) {
        String s = DatatypeConverter.printHexBinary(data);
        System.out.println("COAPHEX:" + s);
    }

    /* Loop on std in to get lines of hex to send */
    public static void main(String[] args) throws IOException {
        InputStream in = System.in;
        final PrintStream out;
        System.err.println("Connecting to " + args[0]);
        if(args.length > 2) {
            Runtime rt = Runtime.getRuntime();
            Process pr = rt.exec(args[2]);
            System.err.println("Started " + args[2]);
            in = pr.getInputStream();
            out = new PrintStream(pr.getOutputStream());
        } else {
            out = System.out;
        }

        /* Create a Hex2UDP that print on this out stream */
        Hex2UDP udpc = new Hex2UDP(args[0], Integer.parseInt(args[1])) {
                public void receive(byte[] data) {
                    String s = DatatypeConverter.printHexBinary(data);
                    out.println("COAPHEX:" + s);
                    out.flush();
                    System.err.println("IN: " + s);
                }
            };

        BufferedReader buffer =
            new BufferedReader(new InputStreamReader(in));

        /* The read loop */
        while(true) {
            String line = buffer.readLine();
            if(line == null) {
                /* Connection closed */
                System.err.println("*** stdin closed");
                System.exit(0);
            } else if (line.startsWith("COAPHEX:")) {
                byte[] data = DatatypeConverter.parseHexBinary(line.substring(8));
                udpc.send(data);
            }
            System.err.println("OUT:" + line);
        }
    }
}
