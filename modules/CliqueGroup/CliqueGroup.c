#include "CliqueGroup.h"

#include <stdlib.h>
#include <stdio.h>

ItemCliquePair* ItemCliquePair_New(void* item){
    ItemCliquePair* pair = malloc(sizeof(ItemCliquePair)); //value of the KeyValuePair struct

    pair->clique = Clique_New();

    pair->item = item;

    List_Append(&pair->clique->similar, pair);

    return pair;
}

void ItemCliquePair_Free(void* value){
    ItemCliquePair* icp = (ItemCliquePair*)value;
    
    free(icp);
}

Clique* Clique_New(){
    Clique* clique = malloc(sizeof(Clique));

    List_Init(&clique->similar);
    List_Init(&clique->nonSimilar);
    clique->nonSimilarHash = NULL; //set as NULL because we won't create and populate it yet

    return clique;
}

void Clique_Free(void* value){
    Clique* clique = (Clique*)value;

    List_Destroy(&clique->similar);
    List_Destroy(&clique->nonSimilar);
    if (clique->nonSimilarHash){
        Hash_Destroy(*(clique->nonSimilarHash));
        free(clique->nonSimilarHash);
    }

    free(clique);
}

void CliqueGroup_Init(CliqueGroup* cg, int bucketSize,unsigned int (*hashFunction)(const void*, unsigned int), bool (*cmpFunction)(void*, void*)){
    Hash_Init(&cg->hash, bucketSize, hashFunction, cmpFunction);
    List_Init(&cg->cliques);
    cg->finalizeNeeded = false;
}

bool CliqueGroup_Add(CliqueGroup* cg, void* key, int keySize, void* value){
    if (Hash_GetValue(cg->hash, key, keySize) != NULL){
        return false;
    }

    ItemCliquePair* icp = ItemCliquePair_New(value); //create icp
    Hash_Add(&(cg->hash), key, keySize, icp);

    List_Append(&(cg->cliques), icp->clique); //append the clique into the list of cliques
    icp->cliqueParentNode = cg->cliques.tail;

    return true;
}

void CliqueGroup_Destroy(CliqueGroup cg){
    /* Frees the entire structure (not the values)*/

    /* destroy hash and free ItemCliquePairs(not the items, just the struct)*/
    Hash_FreeValues(cg.hash, ItemCliquePair_Free);
    Hash_Destroy(cg.hash);
    /* Delete lists inside cliques list and destroy cliques list(which is on stack so no free) */
    List_FreeValues(cg.cliques, Clique_Free);
    List_Destroy(&(cg.cliques));
}

void CliqueGroup_FreeValues(CliqueGroup cg, void (*subFree)(void*)){
    /* Free all items in every list in cliques list*/
    Node* tempNode1 = cg.cliques.head;
    while (tempNode1 != NULL){
        List* insideList = (List*)(tempNode1->value);
        Node* tempNode2 = insideList->head;
        while (tempNode2 != NULL){
            ItemCliquePair* icp = (ItemCliquePair*)(tempNode2->value);
            subFree(icp->item);
            tempNode2 = tempNode2->next;
        }
        tempNode1 = tempNode1->next;
    }
}

bool CliqueGroup_Update_Similar(CliqueGroup* cg, void* key1, int keySize1, void* key2, int keySize2){
    cg->finalizeNeeded = true;

    ItemCliquePair* icp1 = Hash_GetValue(cg->hash, key1, keySize1);
    if(icp1 == NULL)
        return false;
    ItemCliquePair* icp2 = Hash_GetValue(cg->hash, key2, keySize2);
    if(icp2 == NULL)
        return false;

    // If both icps point to the same list then they are already in the same clique.
    // So no further changes should be made.
    if(icp1->clique == icp2->clique){
        return true;
    }

    Clique* mergedCliques = Clique_New();

    /* NOTE: this is why we need the parent node in the ItemCliquePair(to remove the nodes from cliques list) */
    /* save old parent nodes to remove them from the cliques list later on, since they will be changed in MergeCliques*/
    Node* oldParentNode1 = icp1->cliqueParentNode;
    Node* oldParentNode2 = icp2->cliqueParentNode;
    /* save old cliques to destroy them after the merge and append is complete */
    Clique* oldClique1 = icp1->clique;
    Clique* oldClique2 = icp2->clique;

    List_Append(&cg->cliques, mergedCliques);
    CliqueGroup_MergeCliques(mergedCliques, *icp1->clique,*icp2->clique, cg->cliques.tail);


    /* remove the old parent nodes from the cliques list */
    List_RemoveNode(&cg->cliques, oldParentNode1);
    List_RemoveNode(&cg->cliques, oldParentNode2);

    /* free the old cliques */
    Clique_Free(oldClique1);
    Clique_Free(oldClique2);

    return true;
}

bool CliqueGroup_Update_NonSimilar(CliqueGroup* cg, void* key1, int keySize1, void* key2, int keySize2) {
    cg->finalizeNeeded = true;
   
    ItemCliquePair* icp1 = Hash_GetValue(cg->hash, key1, keySize1);
    if(icp1 == NULL)
        return false;
    ItemCliquePair* icp2 = Hash_GetValue(cg->hash, key2, keySize2);
    if(icp2 == NULL)
        return false;

    // We take care of duplicates after CliqueGroup_Finalize.
    List_Append(&icp1->clique->nonSimilar, icp2);
    List_Append(&icp2->clique->nonSimilar, icp1);

    return true;
}

void CliqueGroup_PrintIdentical(CliqueGroup* cg, void (*Print)(void* value)){
    Node* currClique = cg->cliques.head;
    while (currClique != NULL){
        // Printing each clique.
        List* currCliqueItems = (List*)(currClique->value);
        if (currCliqueItems->size > 1){ // Only printing cliques that contain 2+ items.

            // Print each similar pair.
            Node* currItemA = currCliqueItems->head;
            while (currItemA != NULL){
                Node* currItemB = currItemA->next;
                while(currItemB != NULL) {
                    ItemCliquePair *icpA = (ItemCliquePair *) (currItemA->value);
                    ItemCliquePair *icpB = (ItemCliquePair *) (currItemB->value);

                    Print(icpA->item);
                    printf(" ");
                    Print(icpB->item);
                    printf("\n");

                    currItemB = currItemB->next;
                }
                currItemA = currItemA->next;
            }

        }
        currClique = currClique->next;
    }
}

void CliqueGroup_MergeCliques(Clique* newClique, Clique clique1, Clique clique2, Node* cliqueParentNode){
    /* merges 2 cliques into one and changes all the pointers of the ItemCliquePairs to the correct ones */
    // TODO: make merging faster and call half the updates on the pointers.

    //for clique1
	Node* temp1 = clique1.similar.head;
	while(temp1 != NULL){
        ItemCliquePair* icp = (ItemCliquePair*)(temp1->value);
        icp->clique = newClique;
        icp->cliqueParentNode = cliqueParentNode;
        List_Append(&newClique->similar, icp);

		temp1 = temp1->next; //next
	}

    //for clique2
	Node* temp2 = clique2.similar.head;
	while(temp2 != NULL){
        ItemCliquePair* icp = (ItemCliquePair*)(temp2->value);
        icp->clique = newClique;
        icp->cliqueParentNode = cliqueParentNode;
        List_Append(&newClique->similar, icp);

		temp2 = temp2->next; //next
	}

	newClique->nonSimilar = List_Merge(clique1.nonSimilar, clique2.nonSimilar);

}

bool pointercmp(void* value1, void* value2){
    return (*(Clique**)value1 == *(Clique**)value2);
}

void CliqueGroup_Finalize(CliqueGroup cg){ //this should run after the CliqueGroup is updated (however many times)
    cg.finalizeNeeded = false;

    List* cliques = &cg.cliques; // list of all cliques

    //for all cliques
    Node* cliqueNode = cliques->head;
    while(cliqueNode != NULL){
        //for each clique
        Clique* clique = (Clique*)cliqueNode->value;
        Node* icpNode = clique->nonSimilar.head;
        Hash* tempHash = malloc(sizeof(Hash));
        //create hash to help with removing icps with the same cliques
        Hash_Init(tempHash, clique->similar.size * 3, cg.hash.hashFunction, pointercmp);
        while(icpNode != NULL){
            ItemCliquePair* icp = (ItemCliquePair*)icpNode->value;
            bool existsInHash = Hash_GetValue(*tempHash, &icp->clique, sizeof(icp->clique));

            if (!existsInHash){
                Hash_Add(tempHash, &icp->clique, sizeof(icp->clique), icp);
            }else{
                Node* icpNext = icpNode->next;
                List_RemoveNode(&clique->nonSimilar, icpNode);
                icpNode = icpNext;
                continue;
            }

            icpNode = icpNode->next; //next icp
        }
        clique->nonSimilarHash = tempHash;

        cliqueNode = cliqueNode->next; //next clique
    }
}

