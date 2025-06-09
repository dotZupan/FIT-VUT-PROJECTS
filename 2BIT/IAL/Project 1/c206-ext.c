/*
 *  Předmět: Algoritmy (IAL) - FIT VUT v Brně
 *  Rozšíření pro příklad c206.c (Dvousměrně vázaný lineární seznam)
 *  Vytvořil: Daniel Dolejška, září 2024
 */

/*vypracoval: xfignam00 (Matúš Fignár) */

#include "c206-ext.h"

bool error_flag;
bool solved;

/**
 * Tato metoda simuluje příjem síťových paketů s určenou úrovní priority.
 * Přijaté pakety jsou zařazeny do odpovídajících front dle jejich priorit.
 * "Fronty" jsou v tomto cvičení reprezentovány dvousměrně vázanými seznamy
 * - ty totiž umožňují snazší úpravy pro již zařazené položky.
 * 
 * Parametr `packetLists` obsahuje jednotlivé seznamy paketů (`QosPacketListPtr`).
 * Pokud fronta s odpovídající prioritou neexistuje, tato metoda ji alokuje
 * a inicializuje. Za jejich korektní uvolnení odpovídá volající.
 * 
 * V případě, že by po zařazení paketu do seznamu počet prvků v cílovém seznamu
 * překročil stanovený MAX_PACKET_COUNT, dojde nejdříve k promazání položek seznamu.
 * V takovémto případě bude každá druhá položka ze seznamu zahozena nehledě
 * na její vlastní prioritu ovšem v pořadí přijetí.
 * 
 * @param packetLists Ukazatel na inicializovanou strukturu dvousměrně vázaného seznamu
 * @param packet Ukazatel na strukturu přijatého paketu
 */
void receive_packet( DLList *packetLists, PacketPtr packet ) {
	if(MAX_PACKET_COUNT == 0){
		return;
	}

	packetLists->activeElement = packetLists->firstElement;
	QosPacketListPtr buffer = malloc(sizeof(QosPacketList));
		if(buffer == NULL){
			return;
		}
	DLList *NewList = malloc(sizeof(DLList));
		if(NewList == NULL){
			return;
		}
	
	//pokiaľ zoznam packetLists je prázdny vytvor prvý zoznam paketov
	if(DLL_IsActive(packetLists) == false){
		DLL_Init(NewList);
		buffer->list = NewList;
		buffer->priority = packet->priority;
		DLL_InsertLast(packetLists, (long)buffer);
		DLL_InsertLast(buffer->list, (long)packet);
		
	}
	else{
		//pokiaľ v zozname packetLists sa nachádzajú zoznamy paketov iteruj kým neprejdeš všetky alebo nejaký ktorý vyhovuje
		buffer = (QosPacketListPtr)packetLists->activeElement->data;
		while(DLL_IsActive(packetLists)){
			
			//pokiaľ priorita nejaké zoznamu packetov je rovnaká ako priorita vkladaného paketu
			if(packet->priority == buffer->priority){
				//skontroluj či počet paketov v zozname nepresiahol maximálny možný počet paketov a v prípade, že presiahol vymaž každý druhý paket
				if(buffer->list->currentLength == MAX_PACKET_COUNT){
					if(MAX_PACKET_COUNT == 1){
						return;
					}
					else{
						DLL_First(buffer->list);
						do{
							DLL_DeleteAfter(buffer->list);
							DLL_Next(buffer->list);
						} while(buffer->list->activeElement != NULL && buffer->list->activeElement->nextElement != NULL);
					}
				}
				//vlož daný paket na koniec zoznamu paketov s rovnakou prioritou
				DLL_InsertLast(buffer->list, (long)packet);
				free(NewList);
				break;
			}

			// pokiaľ priorita paketu je väčšia ako priorita súčasného zoznamu paketov, vytvor nový zoznam paketov v poradí pred daným zoznamom paketov
			else if(packet->priority > buffer->priority){
				QosPacketListPtr buffer_insert = malloc(sizeof(QosPacketList));
				if(buffer_insert == NULL){
					return;
				}
				DLL_Init(NewList);
				buffer_insert->list = NewList;
				buffer_insert->priority = packet->priority;
				DLL_InsertBefore(packetLists, (long)buffer_insert);
				DLL_InsertLast(buffer_insert->list, (long)packet);	
				break;
				}
			// Inak posuň ukazateľ na následujúci zoznam paketov
			else{
				DLL_Next(packetLists);
				buffer = (QosPacketListPtr)packetLists->activeElement->data;
			}
		}
			// V prípade, že sa nenašla žiadna zhoda počas iterácie vlož vytvor nový zonam paketov nakoniec zoznamu packetLists
		if(!DLL_IsActive(packetLists)){
			QosPacketListPtr buffer_insert = malloc(sizeof(QosPacketList));
			if(buffer_insert == NULL){
				return;
			}
			DLL_Init(NewList);
			buffer_insert->list = NewList;
			buffer_insert->priority = packet->priority;
			DLL_InsertLast(packetLists, (long)buffer_insert);
			DLL_InsertLast(buffer_insert->list, (long)packet);
			}
		}
	}

/**
 * Tato metoda simuluje výběr síťových paketů k odeslání. Výběr respektuje
 * relativní priority paketů mezi sebou, kde pakety s nejvyšší prioritou
 * jsou vždy odeslány nejdříve. Odesílání dále respektuje pořadí, ve kterém
 * byly pakety přijaty metodou `receive_packet`.
 * 
 * Odeslané pakety jsou ze zdrojového seznamu při odeslání odstraněny.
 * 
 * Parametr `packetLists` obsahuje ukazatele na jednotlivé seznamy paketů (`QosPacketListPtr`).
 * Parametr `outputPacketList` obsahuje ukazatele na odeslané pakety (`PacketPtr`).
 * 
 * @param packetLists Ukazatel na inicializovanou strukturu dvousměrně vázaného seznamu
 * @param outputPacketList Ukazatel na seznam paketů k odeslání
 * @param maxPacketCount Maximální počet paketů k odeslání
 */
void send_packets( DLList *packetLists, DLList *outputPacketList, int maxPacketCount ) {
	int send = 0;
	DLL_First(packetLists);

	while (DLL_IsActive(packetLists) && send < maxPacketCount) {
		QosPacketListPtr sending_buffer = (QosPacketListPtr) packetLists->activeElement->data;
		DLL_First(sending_buffer->list);
		//iteruj a posieľaj všetky pakety v danej zložke
        while(sending_buffer->list->firstElement != NULL && send < maxPacketCount) {
            DLL_InsertLast(outputPacketList, (long)sending_buffer->list->firstElement->data);
            DLL_DeleteFirst(sending_buffer->list);
            send++;
        }
		DLL_Next(packetLists);
        
    }
}