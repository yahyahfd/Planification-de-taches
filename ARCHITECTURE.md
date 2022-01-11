#### Compilation

`make` crée à la racine du dépot les executables `cassini` et `saturnd` qui représentent le client et le démon.
`make distclean` sert à supprimer ces executables.

#### Architecture et Execution

Nous avons choisi de créer deux fichier cassini.c et saturnd.c représentant respectivement le client et le démon. Il faudra lancer `./saturnd <chemin pipes>` dans un premier terminal et `./cassini [options]` dans un second afin de commencer la communication entre les deux processus. Le démon attend indéfiniment que le client émette une requête avant d'y répondre en fonction de ce qui a été demandé et les deux se terminent une fois ce processus fini, après avoir écris dans les pipes correspondants les résultats attendus. Les résultats sont faussés dans la plupart des options à cause de l'option create qui n'incrémente pas correctement le taskid après chaque création (nous n'avons pas eu le temps de remédier à ce problème).
