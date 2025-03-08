using System;
using System.IO.Ports;
using System.Threading;

namespace RTCSyncTerminal
{
    class Program
    {

        static SerialPort Seriale1, Seriale2;
        static string Porta1, Porta2;
        static int STimeout = 7000;

        static void Main(string[] args)
        {
            bool check = true;
            string CR;

            Console.BackgroundColor = ConsoleColor.Black;
            Console.ForegroundColor = ConsoleColor.White;


            Console.WriteLine("*****************************************");
            Console.WriteLine("*\t\t\t\t\t*");
            Console.WriteLine("*\t\tRTC Sync\t\t*");
            Console.WriteLine("*\t\t\t\t\t*");
            Console.WriteLine("*****************************************\n");

            do
            {
                do
                {
                    Console.Write("=> Inserire il numero della prima COM: ");
                    CR = Console.ReadLine();

                    if (!int.TryParse(CR, out int r))
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("=> Carattere errato");
                        Console.ForegroundColor = ConsoleColor.White;
                    }
                    else
                        check = false;

                } while (check);

                Porta1 = "COM" + CR;

                Seriale1 = new SerialPort(Porta1);

                if (!Seriale1.IsOpen)
                {
                    try
                    {
                        Seriale1.ReadTimeout = STimeout;
                        Seriale1.DtrEnable = false;
                        Seriale1.BaudRate = 9600;
                        Seriale1.DataBits = 8;
                        Seriale1.Parity = Parity.None;
                        Seriale1.StopBits = StopBits.One;
                        Seriale1.Open();
                    }
                    catch
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("=> Numero porta inesistente o in uso");
                        Console.ForegroundColor = ConsoleColor.White;
                        check = true;
                    }
                }
            } while (check);

            check = true;

            do
            {
                do
                {
                    Console.Write("=> Inserire il numero della seconda COM ('s' per usare solo la prima): ");
                    CR = Console.ReadLine();

                    if (!int.TryParse(CR, out int r))
                    {
                        if (CR.Equals("s"))
                            check = false;
                        else
                        {
                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine("=> Carattere errato");
                            Console.ForegroundColor = ConsoleColor.White;
                        }
                    }
                    else
                        check = false;

                } while (check);

                if (!CR.Equals("s"))
                {
                    Porta2 = "COM" + CR;

                    Seriale2 = new SerialPort(Porta2);

                    if (!Seriale2.IsOpen)
                    {
                        try
                        {
                            Seriale2.ReadTimeout = STimeout;
                            Seriale2.DtrEnable = true;
                            Seriale2.Open();
                        }
                        catch
                        {
                            Seriale2 = null;
                            Console.ForegroundColor = ConsoleColor.Red;

                            if (Porta2.Equals(Porta1))
                                Console.WriteLine("=> Numero porta uguale alla precedente");
                            else
                                Console.WriteLine("=> Numero porta inesistente o in uso");

                            Console.ForegroundColor = ConsoleColor.White;
                            check = true;
                        }
                    }
                }
            } while (check);

            RTCSyncCommand();
        }


        static void RTCSyncCommand()
        {
            string serialMessage;

            DateTime Now;
            string strTime;

            Console.WriteLine("\n=> Invio dati a " + Porta1);
            Console.WriteLine("=> Attendo risposta...");


            Seriale1.DiscardInBuffer();
            Seriale1.Write("RTCSYNC");


            try
            {
                serialMessage = Seriale1.ReadLine();
                //Console.WriteLine(serialMessage + " " + serialMessage.Length);

                if (serialMessage.Equals("RTCSYNC_ACK"))
                {
                    Console.WriteLine("=> " + Porta1 + " Comando ricevuto");

                    Now = DateTime.Now;

                    strTime = Now.Year.ToString("0000") +            // anno
                              "|" + Now.Month.ToString("00") +       // mese
                              "|" + Now.Day.ToString("00") +         // giorno
                              "|" + Now.Hour.ToString("00") +        // ore
                              "|" + Now.Minute.ToString("00") +      // minuti
                              "|" + Now.Second.ToString("00");      // secondi

                    Seriale1.Write(strTime);

                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine("=> " + Porta1 + " Tempo di riferimento: " + Now);
                }

                serialMessage = Seriale1.ReadLine();
                //Console.WriteLine(serialMessage + " " + serialMessage.Length);

                if (serialMessage.Equals("RTCSYNC_OK"))
                {
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.Write("=> "+ Porta1 + " Sincronizzazione completata ");

                    serialMessage = Seriale1.ReadLine();

                    Console.WriteLine(" alla data: " + serialMessage);
                }
            }
            catch (TimeoutException)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("=> Errore: nessun messaggio di risposta ricevuto dalla " + Porta1);
                Console.WriteLine("=> Sincronizzazione fallita.");
            }
            catch (Exception e)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("=> Errore: " + e.Message);
            }

            if (Seriale1.IsOpen)
                Seriale1.Close();


            //Thread.Sleep(3000);

            if (Seriale2 != null)
            {
                Console.ForegroundColor = ConsoleColor.White;
                Console.WriteLine("\n=> Invio dati a " + Porta2);
                Console.WriteLine("=> Attendo risposta...");


                Seriale2.DiscardInBuffer();
                Seriale2.Write("RTCSYNC");

                try
                {
                    serialMessage = Seriale2.ReadLine();
                    //Console.WriteLine(serialMessage + " " + serialMessage.Length);

                    if (serialMessage.Equals("RTCSYNC_ACK"))
                    {
                        Console.WriteLine("=> " + Porta2 + " Comando ricevuto");

                        Now = DateTime.Now;

                        strTime = Now.Year.ToString("0000") +            // anno
                                  "|" + Now.Month.ToString("00") +       // mese
                                  "|" + Now.Day.ToString("00") +         // giorno
                                  "|" + Now.Hour.ToString("00") +        // ore
                                  "|" + Now.Minute.ToString("00") +      // minuti
                                  "|" + Now.Second.ToString("00");      // secondi

                        Seriale2.Write(strTime);

                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine("=> " + Porta2 + " Tempo di riferimento: " + Now);
                    }

                    serialMessage = Seriale2.ReadLine();
                    //Console.WriteLine(serialMessage + " " + serialMessage.Length);

                    if (serialMessage.Equals("RTCSYNC_OK"))
                    {
                        Console.ForegroundColor = ConsoleColor.Green;
                        Console.Write("=> " + Porta2 + " Sincronizzazione completata ");

                        serialMessage = Seriale2.ReadLine();

                        Console.WriteLine(" alla data: " + serialMessage);
                    }
                }
                catch (TimeoutException)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("=> Errore: nessun messaggio di risposta ricevuto dalla " + Porta2);
                    Console.WriteLine("=> Sincronizzazione fallita.");
                }
                catch (Exception e)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("=> Errore sconosciuto: " + e.Message);
                }

                if (Seriale2.IsOpen)
                    Seriale2.Close();
            }

            Console.ForegroundColor = ConsoleColor.White;
            Console.WriteLine("\nPremere un tasto per chiudere il programma...");

            Console.ReadKey();
        } 
    }
}
