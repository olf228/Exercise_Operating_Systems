#include <stdio.h>
#include <semLib.h>
#include <tasklib.h>

const int MAX_CNT = 100*1000*1000;
int count = 0;


int app(int anz, SEM_ID *sem){
	
	int i;
	
	/* Inkrementiere die Variable solange, sie nicht die Konstante MAX_CNT ueberschreitet */
	
	for(i = 0;i<anz; i++){
		
		/* Versuche den kritischen BEreich zu betreten, falls die Semaphore ihn nicht blockt. Ansonsnten soll der Thread warten, bis die Sempahore ihn wieder freigibt*/
		
		semTake(*sem, WAIT_FOREVER);
		if(count >= MAX_CNT) break;
		count++;
		
		/*Zeige aktuellen Fortschritt*/
		
		printf("%d\n", count);
		
		/* Gebe den kritischen Bereich wieder frei */
		
		semGive(*sem);
	}
}

int initApp(){
	
	/* Erstelle 3 Task IDs für die 3 Threads zum Inkrementieren der Variable*/
	
	TASK_ID id1, id2, id3;
	
	/* Erstelle eine ID für die Semaphore zur Ueberwachung des kritischen Abschnitts*/
	
	SEM_ID sem;
	
	/* Erstelle eine Sempahore und weise sie der zuvor erstellten Sempahoren ID zu*/
	
	sem = semBCreate(SEM_Q_FIFO, SEM_FULL);
	
	/* Erstelle 3 Threads, wovon einer bis 50000000 und die anderen beiden jeweils 25000000 zaehlen.*/
	
	id1 = taskCreate("FCT1", 100, 0, 0, (FUNCPTR)app,50000000,&sem,0,0,0,0,0,0,0,0);
	id2 = taskCreate("FCT2", 100, 0, 0, (FUNCPTR)app,25000000,&sem,0,0,0,0,0,0,0,0);
	id3 = taskCreate("FCT3", 100, 0, 0, (FUNCPTR)app,25000000,&sem,0,0,0,0,0,0,0,0);
	
	/* Starte alle Threads*/
	
	taskActivate(id1);
	taskActivate(id2);
	taskActivate(id3);
	
	/* Pausiere initApp solange alle Threads noch arbeiten (inkrementieren der Variable count)*/
	
	taskWait(id1, WAIT_FOREVER);
	taskWait(id2, WAIT_FOREVER);
	taskWait(id3, WAIT_FOREVER);
	
	/* Gebe die globale Variable 'count' aus*/
	
	printf("%d\n", count);
	
	/* Loesche zur Vollstaendigkeit die Sempahore zum Ende des Programms*/
	
	semDelete(sem);

}
