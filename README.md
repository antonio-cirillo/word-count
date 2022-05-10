# :orange_book: Word Count

<div id="top"></div>

[![MIT License][license-shield]][license-url]

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Tabella dei contenuti</summary>
  <ol>
    <li>
      <a href="#introduzione-al-problema">Introduzione al problema</a>
    </li>
    <li>
      <a href="#approccio-al-problema">Approccio al problema</a>
      <ul>
        <li><a href="#soluzione-ad-alto-livello">Soluzione ad alto livello</a></li>
      </ul>
    </li>
    <li><a href="#struttura-del-progetto">Struttura del progetto</a></li>
    <li><a href="#implementazione">Implementazione</a></li>
  </ol>
</details>

<!-- INTRODUZIONE AL PROBLEMA -->
## Introduzione al problema

<p align="right">(<a href="#top">torna su</a>)</p>

<!-- APPROCCIO AL PROBLEMA -->
## Approccio al problema

Uno degli obiettivi fondamentali del progetto è sicuramente quello di distribuire in modo equo il carico di lavoro, in modo da minimizzare il più possibile l'esecuzione di codice sequenziale. In altre parole, è necessario distribuire in modo equo la lettura dei files sui diversi processi a disposizione.

Un approccio immediato potrebbe essere quello di distribuire il numero di files per i processi a disposizione. Questa soluzione, per quanto semplice, risulta essere poco efficiente per due motivi:
* il numero di processi potrebbe essere superiore rispetto al numero di files da leggere;
* la dimensione dei diversi files da leggere potrebbe variare anche di diversi ordini di grandezza.

Adottare questa soluzione porterebbe:
* nel primo caso ad utilizzare solo un sottoinsieme dei processi a disposizione;
* nel secondo caso ad una limitazione dello speed-up: la presenza di files molto più grandi rispetto ad altri implicherebbe tempi di esecuzione molto più lunghi per alcuni processi.

La seconda motivazione apre le porte a quella che è una soluzione molto più efficiente rispetto a quella banale. Invece di dividere il numero di files per i processi a disposizione, possiamo dividere il numero totale di bytes da leggere. In questo modo, la presenza o meno di files con dimensione diversa fra loro non comprometterebbe in nessun modo una divisione equa fra i processi.

### Soluzione ad alto livello

La soluzione è stata implementata tramite l'utilizzo di un'architettura **master-slaves**. In base al ruolo del processo all'interno dell'architettura, è possibile dividere la soluzione in diverse fasi, di seguito riportate.

Il processo **master** si occupa di:
* controllo dell'input dell'utente; 
* calcolo del numero totale di bytes dei files ricevuti in input;
* invio delle porzioni di files da leggere agli **slaves**;
* ricezione degli istogrammi locali degli **slaves**;
* unione dei risultati ricevuti;
* ordinamento dell'istrogramma in ordine decrescente in base al numero di occorrenze;
* creazione di un file csv con le parole e le relative occorrenze.

I processi **slaves** si occupano di:
* ricezione delle porzioni di files da leggere;
* conteggio delle occorrenze delle diverse parole all'interno dei files;
* invio dell'istogramma locale al **master**.

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

Di seguito viene riportata l'implementazione relativa ad ogni fase della soluzione. Alcuni dei listati riportati di seguito non sono completi, in quanto l'obiettivo principale è quello di mostrare solo le parti fondamentali.

### Controllo dell'input

L'operazione di controllo sull'input avviene tramite la funzione `check_input()` della libreria `input.h`. La funzione esegue semplicemente dei controlli sugli argomenti passati in input al programma. I possibili utilizzi del programma sono descritti nella sezione: [modalità di utilizzo](#modalità-di-utilizzo).

### Calcolo dei bytes totali

L'operazione di lettura dei files e calcolo dei bytes totali è stata implementata tramite la funzione `read_files()`, anche essa della libreria `input.h`. Quest'ultima, come mostrato nel listato successivo, prende in input un riferimento ad una variabie di tipo `GList` la quale, al termine della funzione, conterrà la lista di files letti, ed una di tipo `off_t` che conterrà invece il numero totale di bytes letti.

``` c
GList *file_list;
off_t total_bytes = 0;

read_files(..., &file_list, &total_bytes);
```

Il tipo `GList` è definito all'interno della libreria `glib.h`, libreria di supporto utilizzata all'interno del progetto. `GList` definisce una struttura dati a lista, utilizzata come detto precedentemente per la memorizzazione dinamica dei files letti. All'interno della lista i files vengono memorizzati tramite il tipo `File`, definito all'interno della libreria `file.h` come segue:

``` c
#define MAX_PATH_LEN 256

typedef struct file {

    char path_file[MAX_PATH_LEN];
    off_t bytes_size;
    long start_offset;
    long end_offset;

} File;
```

La definizione di questo tipo risulta necessaria per tenere traccia del path di ogni file e la sua relativa grandezza. I parametri `start_offset` e `end_offset` verrano descritti nella sezione successiva.

### Divisione delle porzioni di files

In questa fase vengono divisi i files in porzioni diverse da inviare agli slaves. Il seguente listato mostra quali operazioni vengono effettuate dai diversi processi.

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

Le prime operazioni di questa fase comprendono la dichiarazione di un puntatore di tipo `File` e di una variabile di tipo `guint` (unsigned int) da parte di tutti i processi. 


<p align="right">(<a href="#top">torna su</a>)</p>

# Modalità di utilizzo

<!-- MARKDOWN LINKS & IMAGES -->
[license-shield]: https://img.shields.io/github/license/othneildrew/Best-README-Template.svg?style=for-the-badge
[license-url]: https://github.com/antonio-cirillo/word-count/blob/main/LICENSE
[product-screenshot]: images/screenshot.png
