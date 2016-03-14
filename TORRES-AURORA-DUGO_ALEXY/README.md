#TP3
##Question 4
cd est une commande "built in shell" et non un programme.
ls l'est. Ainsi, ors du cd, le working dirrectory ve être changé en particulier avec les ".." de fin de chemin.
Ce qui correspond au dossier où se situe "symbolicLinkToProj" et non au dossier parent du dossier lié par "symbolicLinkToProj".
Alors que ls, lui, va se déplacer dans le dossier lié par "symbolicLinkToProj" et afficher le ocntenu de son dossier parent.

#TP4
##Question 3
Au delà de l'attente non effectuée, sans waitpid et si le processus enfant se termoine, le système ne libèrera pas les ressources
empruntées par celui-ci et il sera considéré comme zombie, si trop de zombies sont présents dans le système, la table des processus
sera saturée et celà empèchera la création de nouveau processus (cependant les noyaux "récents" résolvent ce problème tout seul)

##Question 4
le signal 2 (norme POSIX) correspond à SIGINT (interruption depuis le clavier)

#TP5
##Question 2
On espère que read, lisant stdin, récupère la sortie de echo "toto" et la place dans la variable a. Enfin le read $a devrait afficher le contenu de la variable a soit "toto". Cependant celà ne se passe pas comme ça. Lors du pipe, un fork se fait. read ne s'execute donc pas dans le shell mais dans un processus fils de celui-ci. Il ne peut pas modifier l'environement du shell et donc ne peut créer ou modifier la variable a. Il va modifier la variable a de son environment qui sera détruit lors de la fin d'execution du read. Enfin on revient dans le shell et on execute le "read $a" qui lit la variable "a" qui n'a jamais été soit créée soit modifiée par le read précédent.

#INFO
Toutes les fonctionnalités demandées (questions + bonus) ont été implémentés.
La gestion des script se fait par un changement du FILE input.
Tous les signaux manipulés (SIGQUIT, SIGINT, SIGCHLD) ont leur propre handler.
J'ai préféré utiliser deux talbeaux descripteurs de fichiers pour implémenter les pipes afin de rendre le code plus simple à comprendre.
