# :orange_book: Word Count

<div id="top"></div>

[![MIT License][license-shield]][license-url]

<!-- TABELLA DEI CONTENUTI -->
<details open>
  <summary>Tabella dei contenuti</summary>
  <ol>
    <li>
      <a href="#introduzione-al-problema">Introduzione al problema</a>
    </li>
    <li>
      <a href="#definizione-della-soluzione">Definizione della soluzione</a>
      <ul>
        <li>
          <a href="#divisione-in-base-al-numero-totale-di-byte">Divisione in base al numero totale di byte</a>
        </li>
        <li>
          <a href="#soluzione-ad-alto-livello">Soluzione ad alto livello</a>
        </li>
      </ul>
    </li>
    <li>
      <a href="#struttura-del-progetto">Struttura del progetto</a>
    </li>
    <li>
      <a href="#implementazione">Implementazione</a>
      <ul>
        <li>
          <a href="#controllo-dellinput">Controllo dell'input</a>
        </li>
        <li>
          <a href="#calcolo-dei-byte-totali">Calcolo dei byte totali</a>
        </li>
        <li>
          <a href="#divisione-delle-porzioni-di-file">Divisione delle porzioni di file</a>
          <ul>
            <li>
              <a href="#invio-file-agli-slave">Invio file agli slave</a>
            </li>
            <li>
              <a href="#ricezione-dei-file-dal-master">Ricezione dei file dal master</a>
            </li>
          </ul>
        </li>
        <li>
          <a href="#conteggio-delle-parole">Conteggio delle parole</a>
        </li>
        <li>
          <a href="#unione-degli-istogrammi-locali">Unione degli istogrammi locali</a>
          <ul>
            <li>
              <a href="#invio-degli-istogrammi-locali">Invio degli istogrammi locali</a>
            </li>
            <li>
              <a href="#ricezione-degli-istogrammi-locali">Ricezione degli istogrammi locali</a>
            </li>
          </ul>
        </li>
      </ul>
    </li>
  </ol>
</details>

<!-- INTRODUZIONE AL PROBLEMA -->
## Introduzione al problema

<p align="right">(<a href="#top">torna su</a>)</p>

<!-- DEFINIZIONE DELLA SOLUZIONE -->
## Definizione della soluzione

Uno degli obiettivi fondamentali del progetto è quello di massimizzare lo speed-up. Al fine di raggiungere tale obiettivo è necessario definire una soluzione efficiente per la distribuzione equa del carico di lavoro. In questo contesto, ciò equivale a distribuire in modo uniforme i file da leggere sui diversi processi.

Un approccio immediato potrebbe essere quello di distribuire il numero di file per i processi a disposizione. Questa soluzione, per quanto semplice, risulta essere poco efficiente per due motivi:
* il numero di processi potrebbe essere superiore rispetto al numero di file da leggere;
* la dimensione dei file potrebbe variare anche di diversi ordini di grandezza.

Adottare questa soluzione porterebbe:
* nel primo caso, ad utilizzare solo un sottoinsieme dei processi a disposizione;
* nel secondo caso, ad una limitazione dello speed-up; la presenza di file molto più grandi rispetto ad altri implicherebbe tempi di esecuzione più lunghi per alcuni processi.

La seconda motivazione apre le porte a quella che è una soluzione molto più efficiente di quella banale: invece di dividere il numero dei file per i processi a disposizione, possiamo dividere il carico di lavoro in base al numero totale di byte da leggere.

### Divisione in base al numero totale di byte

In questa sezione si formalizza la strategia utilizzata per distribuire il lavoro tra i vari processi.

In un primo momento viene calcolato il numero totale di byte. In seguito, lo si divide per il numero di processi a disposizione, in modo da ottenere la quantità di byte destinata ad ognuno di essi. Potrebbe accadere, però, che il numero di processi non sia un multiplo del numero totale di byte. Questo comporterebbe una mancata distribuzione degli ultimi `rest` byte da leggere, dove `rest` è proprio il resto della divisione. Banalmente, si potrebbe pensare di delegare la lettura di questi ultimi `rest` byte ad un unico processo. Tuttavia, una strategia più efficiente risulta essere quella di incaricare i primi `rest` processi di leggere un byte in più, in modo da non sovraccaricare l'utilizzo di un processo rispetto ad un altro.

Quindi, siano p<sub>1</sub>, p<sub>2</sub>, ..., p<sub>n</sub> i processi a disposizione e siano `size` e `rest` relativamente il risultato e il resto della divisione tra il numero totale di byte e il numero di processi, diremo che il processo p<sub>i</sub> sarà incaricato di leggere: 
* `size + 1` byte, se `i` è minore o uguale di `rest`;
* `size` byte, se `i` è maggiore di `rest`. 

### Soluzione ad alto livello

La soluzione è stata implementata tramite l'utilizzo di un'architettura **master-slaves**. In base al ruolo del processo all'interno dell'architettura è possibile dividere la soluzione in diverse fasi, di seguito riportate.

Il processo **master** si occupa di:
* controllare l'input del programma; 
* calcolare il numero totale di byte dei file;
* inviare le porzioni di file da leggere agli **slave**;
* ricevere gli istogrammi locali dagli **slave**;
* unire i risultati ricevuti in un unico istrogramma;
* ordinare l'istrogramma in ordine decrescente in base al numero di occorrenze;
* creare un file csv con le parole e le relative occorrenze.

I processi **slave** si occupano di:
* ricevere le porzioni di file da leggere;
* contare le occorrenze delle diverse parole all'interno dei file;
* inviare l'istogramma locale al **master**.

<p align="right">(<a href="#top">torna su</a>)</p>

## Struttura del progetto

```
.
├── include
│   ├── file.h
│   ├── input.h
│   ├── log.h
│   ├── my-mpi.h
│   ├── sort.h
├── lib
│   ├── file.c
│   ├── input.c
│   ├── log.c
│   ├── my-mpi.c
│   ├── sort.c
├── src
│   ├── word-count.h
├── test
├── LICENSE
├── Makefile
├── README.md
├── install.sh
```

<p align="right">(<a href="#top">torna su</a>)</p>

## Implementazione

In questa sezione viene descritta l'implementazione relativa ad ogni fase della soluzione. Alcuni dei listati riportati di seguito non sono completi, in quanto l'obiettivo principale è quello di mostrare solo le parti fondamentali.

Come supporto per l'implementazione è stata utilizzata la libreria [glib.h](https://docs.gtk.org/glib/). In particolare, sono state utilizzate le seguenti strutture dati:
* `GList`: definisce una struttura di tipo lista utilizzata per la memorizzazione dinamica delle informazioni relative ai file;
* `GHashTable`: implementazione di un hash table per la creazione degli istogrammi;
* `GHashTableIter`: utilizzata per poter iterare le coppie (lessema, occorrenze) memorizzate all'interno dell'hash table.

### Controllo dell'input

L'operazione di controllo dell'input avviene tramite la funzione `check_input()` della libreria `input.h`. La funzione esegue semplicemente dei controlli sugli argomenti passati in input al programma. I possibili input di quest'ultimo vengono descritti nella sezione: [modalità di utilizzo](#modalità-di-utilizzo).

### Calcolo dei byte totali

L'operazione di lettura dei file e di calcolo dei byte totali è stata implementata tramite la funzione `read_files()`, anch'essa della libreria `input.h`. Quest'ultima, come mostrato nel listato successivo, prende in input:
* un riferimento ad una variabie di tipo `GList` la quale, al termine dell'esecuzione della funzione, conterrà la lista di file letti;
* un riferimento ad una variabile di tipo `off_t` che conterrà il numero totale di byte letti.

``` c
GList *file_list;
off_t total_bytes = 0;

read_files(..., &file_list, &total_bytes);
```

All'interno della lista, le informazioni relative ai file vengono memorizzate tramite il tipo `File`, definito all'interno della libreria `file.h` come segue:

``` c
#define MAX_PATH_LEN 256

typedef struct file {

    char path_file[MAX_PATH_LEN];
    off_t bytes_size;
    long start_offset;
    long end_offset;

} File;
```

La definizione di questa struttura permette la memorizzazione dei path assoluti e la dimensione di ogni file. I parametri `start_offset` e `end_offset` verranno inizializzati successivamente dal processo master per definire la parte di file da delegare ad ogni processo.

### Divisione delle porzioni di file

In questa fase si definiscono le porzioni di file da leggere da parte dei vari slave. Il seguente listato mostra quali operazioni vengono effettuate dai diversi processi.

``` c
File *files;
guint n_files;          

MPI_Datatype file_type = create_file_type();

if (rank == MASTER) {

    send_files_to_slaves(size, file_list, total_bytes, file_type);

} else {

    recv_files_from_master(&n_files, &files, file_type);

}

MPI_Type_free(&file_type);
```

Le prime operazioni di questa fase comprendono la dichiarazione di un puntatore di tipo `File`, buffer utilizzato dagli slave per ricevere le informazioni relative ai file da leggere, e di una variabile di tipo `guint` (alias `unsigned int`) che conterrà il numero di file che riceverà dal master.

Successivamente, viene eseguita la funzione `create_file_type()`, della libreria `my-mpi.h`, da parte di tutti i processi. Quest'ultima viene utilizzata per la creazione del tipo derivato `File` in modo da permettere ai processi di ricevere e inviare variabili di questo tipo. Il risultato di questa funzione viene memorizzato all'interno della variabile `file_type` di tipo `MPI_Datatype`.

Una volta creato il tipo `File`, i processi eseguono una funzione diversa in base al loro ruolo all'interno dell'architettura:
* master esegue la funzione `send_files_to_slaves()`;
* gli slave eseguono la funzione `recv_files_from_master()`.

Entrambe le funzioni sono state implementate all'interno della libreria `my-mpi.h`.

Terminato l'invio e la ricezione delle informazioni, viene eseguita la funzione `MPI_Type_free()` da parte di tutti i processi in modo da eliminare il tipo `file_type`. 

#### Invio file agli slave

La funzione `send_files_to_slaves()`, come suggerisce il nome, viene utilizzata dal master per comunicare agli slave le diverse porzioni di file da leggere.  
Le prime operazioni sono relative al calcolo del risultato e del resto fra il numero totale di byte e il numero di slave.

``` c
int n_slaves = size - 1;

off_t bytes_for_each_processes = total_bytes / n_slaves;
off_t rest = total_bytes % n_slaves;
```

Successivamente, viene allocato dinamicamente il buffer relativo ai file da inviare ad ogni processo. Il buffer avrà una dimensione pari al valore di `n_files` che, tramite l'utilizzo della funzione `g_list_length()`, conterrà il numero totale di file letti. Il valore di `n_files` rappresenta un upper-bound al numero reale di file che ogni processo dovrà leggere. Infatti, sarà necessario comunicare ad ogni slave il numero effettivo di file che riceverà in modo da permettere ad ognuno di essi di allocare spazio a sufficenza per ricevere correttamente il messaggio.

``` c
guint n_files = g_list_length(file_list);
File *files = malloc((sizeof *files) * n_files);
```

Poiché le varie porzioni da destinare ad ogni processo vengono calcolate volta per volta, si è deciso di utilizzare una comunicazione non-bloccante. Questo permette al master di continuare a dividere i file tra i vari processi e contemporaneamente inizializzare la comunicazione verso il processo sul quale sono state appena calcolate le porzioni da leggere.  
L'utilizzo della comunicazione non-bloccante richiede che il buffer utillizzato all'interno della comunicazione non venga in nessun modo modificato fin quando la comunicazione non viene completata. Per questo motivo, viene dichiarata una matrice `n_slaves` x `BUFFER_SIZE`, dove la riga `i` rappresenta un buffer di grandezza `BUFFER_SIZE` utilizzato dal master per inviare i dati all'i-esimo slave. `BUFFER_SIZE` viene definito all'interno della libreria `my-mpi.h` ed ha un valore pari ad 8096.  
Inoltre, viene dichiarato anche un array di tipo `MPI_Request` di dimensione `n_slaves`, utilizzato per la memorizzazione delle richieste relative ad ogni comunicazione. Quest'ultimo sarà necessario per attendere il completamento di tutte le comunicazioni effettuate.

``` c
char buffers[n_slaves][BUFFER_SIZE];
MPI_Request requests[n_slaves];
```

Il prossimo passo consiste nel calcolare, per ogni processo, il numero di byte da inviare. Una volta fatto ciò, finché il numero di byte da inviare è maggiore di zero, vengono effettuate le seguenti operazioni:
1. Copia della struttura `File` attualmente puntata all'interno della lista;
2. Calcolo del numero di byte non distribuiti del file attuale;
3. Inizializzazione dei campi `start_offset` ed `end_offset`;
4. Copia della struttura appena modificata all'interno del buffer `files`;
5. Aggiornamento del numero di byte ancora da inviare al processo corrente;
6. Se il file attuale non ha più byte rimanenti da distribuire:
    * viene aggiornato il puntatore della lista al file successivo e si torna al passo 1, altrimenti
    * si memorizza il valore relativo all'ultimo offset letto del file attuale e si passa alla partizione dei file per il processo successivo.

``` c
for (int i_slave = 0; i_slave < n_slaves; i_slave++) {

  off_t total_bytes_to_send = bytes_for_each_processes;
  if (rest > 0) {
    total_bytes_to_send++;
    rest--;
  }

  while (total_bytes_to_send > 0) { ... }
```

Terminata la partizione dei file relativa al processo corrente, tramite la funzione `MPI_Pack()`, impachettiamo all'interno del buffer `&buffers[0][i_slave]`, dove `i_slave` indica l'indice del processo corrente, il numero di file memorizzati all'interno del buffer `files` e il buffer `files` stesso. Il numero di file da inviare al processo corrente equivale al valore della variabile `i_file`, utilizzata per tenere traccia della prossima cella libera all'interno del buffer `files`.

```
  guint n_files = i_file;

  int position = 0;
   
  MPI_Pack(&n_files, 1, MPI_UNSIGNED, 
    &buffers[0][i_slave], BUFFER_SIZE, 
    &position, MPI_COMM_WORLD);
    
  MPI_Pack(files, i_file, file_type, 
    &buffers[0][i_slave], BUFFER_SIZE, 
    &position, MPI_COMM_WORLD);
```

Una volta terminato l'impachettamento dei dati non ci resta che inviarli. Gli slave, non avendo altre operazioni da effettuare prima della ricezione delle informazioni da parte del master, saranno sicuramente già in attessa del messaggio. Per questo motivo, possiamo utilizzare una comunicazione non-bloccante di tipo **ready**, tramite l'utilizzo della funzione `MPI_Irsend()`. Quest'ultima aggiunge un'informazione in più rispetto ad una send standard, permettendo ad MPI di eliminare l'operazione di hand-shake, e quindi ottenere un miglioramento delle presetazione.

```c
  MPI_Irsend(&buffers[0][i_slave], BUFFER_SIZE,
    MPI_PACKED, i_slave + 1, TAG_NUM_FILES, 
    MPI_COMM_WORLD, &requests[i_slave]);

}
```

Una volta che le comunicazioni per tutti i processi sono state inizializzate, viene eseguita la funzione `MPI_Waitall()` in modo attendere il completamento di quest'ultime. 

``` c
MPI_Waitall(n_slaves, requests, MPI_STATUS_IGNORE);
```

#### Ricezione dei file dal master

La funzione `recv_files_from_master()` prende in input il riferimento ad una variabile di tipo `guint` (`n_files`) e un riferimento ad un puntatore di tipo `File` (`files`). La funzione viene utilizzate per ricevere le porzioni di file da leggere inviate dal master e memorizzare il numero di file e i file nelle due variabili passate in input alla funzione. 

Come mostrato nel listato successivo, viene invocata la funzione `MPI_Recv()` e successivamente la funzione `MPI_Unpack` per ricevere e spacchettare il messaggio. 

``` c
char *buffer = malloc((sizeof *buffer) * BUFFER_SIZE);
    
MPI_Recv(buffer, BUFFER_SIZE, MPI_PACKED, MASTER,
  TAG_NUM_FILES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

int position = 0;
   
MPI_Unpack(buffer, BUFFER_SIZE, &position, 
  n_files, 1, MPI_UNSIGNED, MPI_COMM_WORLD);
  
*files = malloc((sizeof **files) * *n_files);
MPI_Unpack(buffer, BUFFER_SIZE, &position, 
  *files, *n_files, file_type, MPI_COMM_WORLD);

free(buffer);
```

### Conteggio delle parole

Ricevute le porzioni di file da leggere da parte del master, gli slave iniziano la fase di conteggio delle parole. Risulta necessario definire una politica di conteggio per prevenire errori dovuti alla partizione dei file in base al numero di byte. I vari slave ricevono come informazione per ogni file il path relativo a quest'ultimo e l'offset iniziale e finale di lettura. A questo punto è possibile trovarsi in due casi:
* l'offset iniziale ha valore pari a zero;
* l'offset iniziale è diverso da zero.

Nel primo caso il processo inizia a leggere il file dall'inizio, mentre nel secondo inizia a leggere il file subito dopo l'ultimo byte letto dal processo precedente. Questa strategia potrebbe in qualche modo troncare la lettura di una parola su processi differenti, causando la lettura di quest'ultima in modo errato. Per prevenire questo problema risulta necessario definire una politica comune a tutti i processi:
* se il processo inizia a leggere dall'offset zero, allora quest'ultimo conterà tutte le parole all'interno della sua porzione di file. Inoltre, se l'ultimo byte letto è un carattere, continua a leggere il file fin quando non termina la lettura della parola attuale;
* se il processo inizia a leggere da un offset diverso da zero, allora quest'ultimo controlla se l'offset attuale coincide con l'inizio di una parola, altrimenti salta tutti i byte iniziali fino all'inizio della parola successiva.

L'utilizzo di questa politica permette di gestire il conteggio relativo alle parole troncate dalla divisione dei file in base al numero totale di byte. In questo modo, il processo corrente continuerà a leggere il file fin quando l'ultima parola non termina, mentre il processo successivo la ignorerà.

Poiché l'obiettivo di questa fase è contare il numero di occorrenze per ogni lessema, risulta necessario controllare ad ogni lettura se la parola appena letta è stata già contata in precedenza oppure no. Per questo motivo, per rendere la ricerca delle parole già contate più efficiente, è stata utilizzata un'hash table. Grazie all'hash table è possibile controllare in tempo costante se la parola attuale sia già presente all'interno di essa oppure no.

Ogni slave esegue la funzione `count_words()`, implementata all'interno della libreria `file.h`, per ogni file ricevuto dal master. Quest'ultima utilizza la politica precedentemente descritta per contare le parole all'interno del file. Le coppie (lessema, occorrenze) vengono memorizzate all'interno della variabile `hash_table` passata come riferimento alla funzione.

``` c
GHashTable *hash_table;

if (rank != MASTER) {

  hash_table = g_hash_table_new(g_str_hash, g_str_equal);

  for (int i = 0; i < n_files; i++) {
  
    File file = files[i];
    count_words(&hash_table, file.path_file, 
      file.start_offset, file.end_offset);
  
  }

}
```
### Unione degli istogrammi locali

In questa fase gli slave comunicano il loro istogramma locale al master in modo da unirli e creare un unico risultato. Per poter comunicare le coppie (lessema, occorrenze) memorizzate all'interno dell'hash table locali di ogni slave, viene definita la seguente struttura all'interno della libreria `file.h`.

``` c
#define MAX_WORD_LEN 128

typedef struct word {

    char lexeme[MAX_WORD_LEN];
    unsigned int occurrences;

} Word;
```

Il listato successivo mostra le operazioni effettuate da ogni processo.

``` c
MPI_Datatype word_type = create_word_type();

if (rank == MASTER) {
       
  hash_table = g_hash_table_new(g_str_hash, g_str_equal);

  recv_words_from_slaves(size, &hash_table, word_type);

} else {

  send_words_to_master(hash_table, word_type);

}

MPI_Type_free(&word_type);
```

Come per la fase di invio delle porzioni di file, anche in questa fase viene creato un tipo derivato per l'invio e la ricezione di variabili di tipo `Word` tramite la funzione `create_word_type()`. Il risultato viene memorizzato all'interno della variabile `word_type` utilizzata dalla due funzioni successive per la comunicazione dei dati.

Successivamente, il processo master esegue la funzione `recv_words_from_slaves()` utilizzata per la ricezione e unione degli istogrammi locali di ogni slave. I risultati vengono memorizzati all'interno della variabile `hash_table` passata come riferimento alla funzione. Invece, i processi slave eseguono la funzione `send_words_to_master()` per l'invio dell'istogramma locale al processo master.

Una volta terminate le comunicazioni viene eseguita la funzione `MPI_Type_free()` in modo da eliminare il tipo derivato `word_type`.

#### Invio degli istogrammi locali

Analizziamo il codice relativo alla funzione `send_words_to_master()` in modo da descrivere tutte le fasi per l'invio dell'istogramma locale.

Per poter iterare le varie coppie memorizzate all'interno dell'hash table viene dichiarata e inizializzata la struttura `GHashTableIter` come segue.

``` c
GHashTableIter hash_table_iter;
g_hash_table_iter_init(&hash_table_iter, hash_table);
```

Successivamente otteniamo il numero di parole memorizzate all'interno dell'hash table tramite la funzione `g_hash_table_size()` e memorizziamo il risultato all'interno della variabile `n_words`.

``` c
guint n_words = g_hash_table_size(hash_table);
```

A questo punto controlliamo se il numero di parole da inviare è uguale a zero. In questo caso comunichiamo tale risultato al master e terminiamo l'esecuzione della funzione.

``` c
if (0 == n_words) {

  MPI_Send(&n_words, 1, MPI_UNSIGNED, MASTER,
    TAG_MERGE_SIZE, MPI_COMM_WORLD);
  
  return ;

}
```
Se invece il numero di parole è maggiore di zero, allora le operazioni da effettuare sono due:
* invio del numero di parole contate;
* invio delle coppie (lessema, occorrenze).

Si potrebbe pensare all'utilizzo della funzione `MPI_Pack()` per impacchettare le due operazioni in una singola comunicazione. In questo caso, però, si preferisce utilizzare due comunicazioni differenti per il seguente motivo: la grandezza del buffer utilizzato per comunicare dati impacchettati deve essere dichiarata a tempo di compilazione in modo da permettere sia al mittente che al destinatario di conoscere la grandezza effettiva del buffer utilizzato durante la comunicazione. L'utilizzo di un valore costante potrebbe portare ad una delle seguenti problematiche:
* lo spazio allocato potrebbe essere molto più grande rispetto alla grandezza necessaria per comunicare i dati;
* lo spazio allocato potrebbe essere inferiore rispetto alla quantità di dati da comunicare.

Queste due problematiche causerebbero:
* nel primo caso ad operazioni di comunicazione molto più onerose rispetto a quelle necessarie;
* nel secondo caso ad una perdita di dati.

Nel nostro caso, la grandezza del buffer utilizzata è pari al valore `BUFFER_SIZE`, ovvero 8192. Le coppie (lessema, occorrenze) vengono memorizzate all'interno della struttura `Word`, la quale occupa all'interno del buffer uno spazio di 132 byte. In questo caso, se un processo memorizza all'interno del suo istogramma locale un numero di parole differenti maggiori di 62, la funzione `MPI_Pack()` causerebbe un errore in quanto la dimensione dei dati da memorizzare supererebbe la dimensione effettiva del buffer. Per questo motivo, si è preferito lasciare le due comunicazioni separate, gestendole tramite una comunicazione non-bloccante.

A questo punto l'operazione successiva consiste nell'allocare un array di tipo `MPI_Request()` di dimensione due. Quest'ultimo verrà utilizzato successivamente per attendere il completamento delle due comunicazioni

``` c
MPI_Request *requests = malloc((sizeof *requests) * 2);
```

Una volta allocato lo spazio necessario per gestire il completamento delle due comunicazioni, inizializziamo la comunicazione relativa al numero di parole che lo slave invierà al master.

``` c
MPI_Isend(&n_words, 1, MPI_UNSIGNED, MASTER,
  TAG_MERGE_SIZE, MPI_COMM_WORLD, &requests[0]);
```

La prossima operazione consiste nell'allocare lo spazio relativo al buffer `words`. All'interno di esso verranno memorizzate le coppie (lessema, occorrenze) tramite l'utilizzo del tipo `Word`.

``` c
Word *words = malloc(sizeof(*words) * n_words);
```

Una volta convertite ogni entry dell'hash table in strutture di tipo `Word` e memorizzate all'interno del buffer `words`, inizializziamo l'invio del buffer al master tramite la funzione `MPI_Isend()`.

``` c
MPI_Isend(words, n_words, word_type, MASTER, 
  TAG_MERGE_STRUCT, MPI_COMM_WORLD, &requests[1]);
```

Infine, utilizziamo la funzione `MPI_Waitall()` per attendere il completamente delle due comunicazioni.

``` c
MPI_Waitall(2, requests, MPI_STATUS_IGNORE);
```

#### Ricezione degli istogrammi locali

Il master in questa fase utilizza la funzione `recv_words_from_slaves()` per ricevere gli istogrammi locali da parte degli slave. Di seguito viene riportata l'implementazione di tale funzione.

Come prima operazione viene inizializzata una ricezione non-bloccante verso tutti i processi slave per l'ottenimento del numero di parole conteggiate. L'utilizzo di una comunicazione non-bloccante deve garantire che il buffer utilizzato durante la comunicazione non venga modificato fin quando quest'ultima non si completi. Per questo motivo, viene allocato l'array `n_words_for_each_processes` utilizzato per ricevere il numero di parole che ogni slave comunicherà al master.
Inoltre, come per le comunicazioni precedenti, allochiamo un array di tipo `MPI_Request` per poter completare successivamente le ricezioni.

``` c
int n_slaves = size - 1;

MPI_Request *requests_merge_size = malloc((sizeof
  *requests_merge_size) * n_slaves);

guint *n_words_for_each_processes = malloc((sizeof
  *n_words_for_each_processes) * n_slaves);

for (int i_slave = 0; i_slave < n_slaves; i_slave++) {
  
  MPI_Irecv(&n_words_for_each_processes[i_slave], 1, 
    MPI_UNSIGNED, i_slave + 1, TAG_MERGE_SIZE, 
    MPI_COMM_WORLD, &requests_merge_size[i_slave]);

}
```

Non appena una qualsiasi ricezione verso un processo `i` viene completata, viene allocato un buffer di dimensione pari al numero di parole ottenuto e inizializzata una nuova comunicazione per l'ottenimente delle parole conteggiate verso il processo `i`. Le prime due operazioni da effettuare sono :
* allocazione di un array di tipo `MPI_Request` utilizzato per il completamento della seconda comunicazione;
* allocazione di un buffer per ogni slave di tipo `Word` per la ricezione delle parole da parte di ogni slave.

``` c
MPI_Request *requests_merge_struct = malloc((sizeof
  *requests_merge_struct) * n_slaves);

Word **buffers = malloc((sizeof **buffers) * n_slaves);
```

Utilizziamo ora la funzione `MPI_Waitany()` per attendere il completamento di una qualsiasi ricezione verso un processo `i`. Quest'ultima prende in input il riferimento ad una variabile `int`, nel nostro caso `index`, e la inizializza con il valore relativo all'indice della ricezione completata all'interno dell'array `requests_merge_size`. Inoltre, manteniamo traccia del numero di slave che invierà un numero di parole maggiore di zero. 

``` c
int n_recvs = n_slaves;

for (int i = 0; i < n_slaves; i++) {

  int index;

  MPI_Waitany(n_slaves, requests_merge_size, 
    &index, MPI_STATUS_IGNORE);

  guint n_words = n_words_for_each_processes[index];

  if (0 == n_words) {

    requests_merge_struct[index] = MPI_REQUEST_NULL;
    n_recvs--;
    continue;
    
  }
``` 
a quest'ultima
Se il valore relativo al numero di parole appena ricevuto è maggiore di zero, allora allochiamo il buffer relativo alla ricezione delle parole e inizializziamo la ricezione verso il processo `index + 1`. Utilizziamo `index + 1` in quanto il valore degli indici parte da zero mentre il `rank` relativo agli slave parte da uno.

``` c
  *(buffers + index) = malloc((sizeof **buffers) * n_words);

  MPI_Irecv(*(buffers + index), n_words, word_type, 
    index + 1, TAG_MERGE_STRUCT, MPI_COMM_WORLD, 
    &requests_merge_struct[index]);


}
```

Utilizzando la stessa ideologia, anon appena una qualsiasi ricezione della lista di parole conteggiate viene completata, inseriamo quest'ultime all'interno dell'istogramma globale.

``` c
for (int i = 0; i < n_recvs; i++) {

  int index;
  
  MPI_Waitany(n_slaves, requests_merge_struct,
    &index, MPI_STATUS_IGNORE);

  for (int j = 0; j < n_words; j++) { 

    Word word = buffers[index][j];

    if (g_hash_table_contains(*hash_table, word.lexeme)) {
            
      ...
      g_hash_table_replace(...));
            
    } else {
      
      ...
      g_hash_table_insert(...);
           
    }

  }

}
```

<p align="right">(<a href="#top">torna su</a>)</p>

# Benchmark

513,67 Megabyte.

| Numero di slave | Tempo di esecuzione in secondi | Speed-up |
| :-------------: | :----------------------------: | :------: |
| 1               | 74.031666                      | 1.00     |
| 2               | 46.204213                      | 1.60     |
| 3               | 38.388234                      | 1.93     |
| 4               | 28.560539                      | 2.59     |
| 5               | 22.948686                      | 3.22     |
| 6               | 19.282333                      | 3.84     |
| 7               | 16.855340                      | 4.39     |
| 8               | 14.260683                      | 5.19     |
| 9               | 12.668596                      | 5.84     |
| 10              | 11.485917                      | 6.45     |
| 11              | 10.939283                      | 6.77     |
| 12              | 10.06692                       | 7.35     |
| 13              | 9.352552                       | 7.92     |
| 14              | 8.699361                       | 8.51     |
| 15              | 7.927089                       | 9.34     |
| 16              | 7.526769                       | 9.84     |
| 17              | 7.228280                       | 10.24    |
| 18              | 6.704542                       | 11.04    |
| 19              | 6.459366                       | 11.46    |
| 20              | 6.051065                       | 12.23    |
| 21              | 5.782785                       | 12.80    |
| 22              | 5.539639                       | 13.36    |
| 23              | 5.338991                       | 13.87    |

Numero di slave x 112,37 Megabyte.

| Numero di slave | Tempo di esecuzione in secondi | Dimensione input |
| :-------------: | :----------------------------: | :--------------: |
| 1               | 15.869725                      | 112,37           |
| 2               | 20.955662                      | 224.74           |
| 3               | 25.055182                      | 337.11           |
| 4               | 25.395312                      | 449.48           |
| 5               | 24.543799                      | 561.85           |
| 6               | 26.670880                      | 674.22           |
| 7               | 26.915306                      | 786.59           |
| 8               | 26.716327                      | 898.96           |
| 9               | 26.589688                      | 1011.33          |
| 10              | 26.551581                      | 1123.7           |
| 11              | 26.568343                      | 1236.07          |
| 13              | 27.124493                      | 1348.44          |
| 13              | 26.975850                      | 1460.81          |
| 14              | 27.053065                      | 1573.18          |
| 15              | 26.598351                      | 1685.55          |
| 16              | 26.635424                      | 1797.92          |
| 17              | 26.477229                      | 1910.29          |
| 18              | 27.163188                      | 2022.66          |
| 19              | 26.503225                      | 2135.03          |
| 20              | 26.652417                      | 2247.4           |
| 21              | 26.390431                      | 2359.77          |
| 22              | 26.490460                      | 2472.14          |        
| 23              | 26.610655                      | 2584.51          |
  
## Esecuzione
### Installazione
### Modalità di utilizzo

<!-- MARKDOWN LINKS & IMAGES -->
[license-shield]: https://img.shields.io/github/license/othneildrew/Best-README-Template.svg?style=for-the-badge
[license-url]: https://github.com/antonio-cirillo/word-count/blob/main/LICENSE
[product-screenshot]: images/screenshot.png
