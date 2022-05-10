# Word Count

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
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
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

<!-- MARKDOWN LINKS & IMAGES -->
[license-shield]: https://img.shields.io/github/license/othneildrew/Best-README-Template.svg?style=for-the-badge
[license-url]: https://github.com/antonio-cirillo/word-count/blob/main/LICENSE
[product-screenshot]: images/screenshot.png
