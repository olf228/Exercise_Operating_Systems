#include <semLib.h>
#include <stdio.h>
#include <taskLib.h>

int essZeit = 10;

SEM_ID printSem;


/*Vorwaertsdeklaration*/
int app( SEM_ID *sem1, SEM_ID *sem2, char *name);

/*Hilfsmethode zum Ausgeben von Strings auf der Konsole */
void log(char *name, char *msg) 
{
    /* Semaphore für Konsoleausgabe, damit die einzelnen Threads nicht gleichzeitig in der Konsole schreiben */
	semTake(printSem, WAIT_FOREVER);
		/* msg ausgeben*/
		printf("%s: %s\n", name, msg);
    /* Sempahore wieder freigeben */
	semGive(printSem);
}

int initApp()
{
    /* Erstelle die Konsolen Sempahore */
	printSem = semBCreate(SEM_Q_FIFO, SEM_FULL);
	
    /* Fuer jeden Loeffel eine Sempahore erstellen */
	SEM_ID spoon1 = semBCreate(SEM_Q_FIFO, SEM_FULL);
	SEM_ID spoon2 = semBCreate(SEM_Q_FIFO, SEM_FULL);
	SEM_ID spoon3 = semBCreate(SEM_Q_FIFO, SEM_FULL);
	
    /* 3 Philsophen sitzen am Tisch, deswegen 3 Tasks erstellen */
	TASK_ID philosoph1 = taskCreate("Philosoph1", 100, 0, 0, (FUNCPTR)app,&spoon1,&spoon3,"Philosoph1",0,0,0,0,0,0,0);
	TASK_ID philosoph2 = taskCreate("Philosoph2", 100, 0, 0, (FUNCPTR)app,&spoon2,&spoon1,"Philosoph2",0,0,0,0,0,0,0);
	TASK_ID philosoph3 = taskCreate("Philosoph3", 100, 0, 0, (FUNCPTR)app,&spoon3,&spoon2,"Philosoph3",0,0,0,0,0,0,0);
	
    /* Philosophen starten */
	taskActivate(philosoph1);
	taskActivate(philosoph2);
	taskActivate(philosoph3);
	
    /* Hauptprogramm bis zum Ende von app() anhalten */
	taskWait(philosoph1, WAIT_FOREVER);
	taskWait(philosoph2, WAIT_FOREVER);
	taskWait(philosoph3, WAIT_FOREVER);
	
    /* Alle Sempahoren werden wieder geloescht */
	semDelete(spoon1);
	semDelete(spoon2);
	semDelete(spoon3);
	semDelete(printSem);
	
}

int app(SEM_ID *sem1, SEM_ID *sem2, char *name)
{
    int waited = 0;
	while(run <1000)
	{
		
    	/* Speichere einen random Integer für die Nachdenkzeit */
		int r = rand() % 10;
		log(name, "Ich denke nach...");
        /* Philosophen philosphieren lassen für die Zeitspanne, die in r gespeichert ist */
		taskDelay(r);
        log(name, "Fertig mit denken.");
        /* Fertig mit dem Philosophieren. Jetzt versuchen etwas zu essen. Allerdings erst warten, bis alle Loeffel frei sind */
        /* Erst testen, ob der Loeffel frei ist. Wenn nicht den waited Counter nach oben zaehlen und dann wieder nachdenken */

        /* Erste Sempahore versuchen zu nehmen */
        if(semTake(*sem1, 0) == ERROR) {
        	waited++;
        	log(name, "Ich konnte den linken Loeffel nicht nehmen! Ich denke wieder nach!");
            continue;
        }
        /* Zweite Sempahore versuchen zu nehmen */
        log(name, "Ich konnte den linken Löffel nehmen.");
        if(semTake(*sem2, 0) == ERROR) {
            waited++;
            log(name, "Ich konnte den linken Loeffel nehmen aber den Rechten nicht! Ich denke wieder nach!");
            semGive(*sem1);
            continue;      
        }
        log(name, "Ich konnte den rechten Löffel nehmen.");
		
		/*Beide Loeffel stehen jetzt zur Verfuegung. Beginne zu essen! */
        log(name, "Beginne zu essen...");
		taskDelay(essZeit);
        
        /* Esszeit vorbei. Beide Loeffel wieder freigeben! */
		log(name, "Jetzt bin ich erstmal fertig\n");
        semGive(*sem1);
        semGive(*sem2);
        run++;
    }
	
	/* Sempahore printsem verwenden, damit nur ein Task gleichzeitig in der Konsole ausgibt */
	semTake(printSem, WAIT_FOREVER);
	/* Warten ausgeben */
		printf("%s: Ich habe %d Mal gewartet\n", name, waited);
	/* Sempahore wieder freigeben */
	semGive(printSem);
}

