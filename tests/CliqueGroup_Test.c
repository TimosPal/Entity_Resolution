#include "CliqueGroup.h"
#include "acutest.h"
#include "Hashes.h"
#include "StringUtil.h"
#include "LinkedList.h"
#include "Item.h"
#include "Hash.h"
#include "JSONParser.h"

#include <stdbool.h>

#define INPUT_FILE "../Datasets/camera_specs/2013_camera_specs/buy.net/4233.json"

void CliqueGroup_Test_Add(){
    CliqueGroup cg;
    CliqueGroup_Init(&cg, 10, RSHash, StringCmp);

    /* insert item1 */
    char item1ID[10] = "test_id1";
    Item* item1 = Item_Create(item1ID, GetJsonPairs(INPUT_FILE));
    CliqueGroup_Add(&cg, item1ID, strlen(item1ID)+1, item1);

    /* cliques list is not empty */
    TEST_ASSERT(cg.cliques.size == 1);

    /* clique for the item exists */
    TEST_ASSERT(cg.cliques.head->value != NULL);

    /* cliques head is not NULL */
    TEST_ASSERT(((List*)(cg.cliques.head->value))->head != NULL);
    List* clique1 = ((List*)(cg.cliques.head->value));

    /* see if item1 is correctly inserted */
    TEST_ASSERT(((ItemCliquePair*)(clique1->head->value))->item == item1);


    /* insert item2 */
    char item2ID[10] = "test_id2";
    Item* item2 = Item_Create(item2ID, GetJsonPairs(INPUT_FILE));
    CliqueGroup_Add(&cg, item2ID, strlen(item2ID)+1, item2);

    /* cliques list size should be 2 */
    TEST_ASSERT(cg.cliques.size == 2);

    /* clique for the item exists (node number 2) */
    TEST_ASSERT(cg.cliques.head->next->value != NULL);

    /* cliques head is not NULL */
    TEST_ASSERT(((List*)(cg.cliques.head->next->value))->head != NULL);
    List* clique2 = ((List*)(cg.cliques.tail->value));

    /* see if item2 is correctly inserted */
    TEST_ASSERT(((ItemCliquePair*)(clique2->head->value))->item == item2);


    /* free memory */
    CliqueGroup_FreeValues(cg, Item_Free);
    CliqueGroup_Destroy(cg);
}

void CliqueGroup_Test_Merge_Cliques(){
    CliqueGroup cg;
    CliqueGroup_Init(&cg, 10, RSHash, StringCmp);

    /* insert item1 */
    char item1ID[10] = "test_id1";
    Item* item1 = Item_Create(item1ID, GetJsonPairs(INPUT_FILE));
    CliqueGroup_Add(&cg, item1ID, strlen(item1ID)+1, item1);

    /* this is the first clique */
    //List* clique1 = ((List*)(cg.cliques.head->value));

    /* insert item2 */
    char item2ID[10] = "test_id2";
    Item* item2 = Item_Create(item2ID, GetJsonPairs(INPUT_FILE));
    CliqueGroup_Add(&cg, item2ID, strlen(item2ID)+1, item2);

    /* this is the sexond clique */
    //List* clique2 = ((List*)(cg.cliques.tail->value));


    ItemCliquePair* icp1 = Hash_GetValue(cg.hash, item1ID, strlen(item1ID)+1);    
    
    /* icp1 shouldn't be NULL */
    TEST_ASSERT(icp1 != NULL);

    ItemCliquePair* icp2 = Hash_GetValue(cg.hash, item2ID, strlen(item2ID)+1);
    
    /* icp2 shouldn't be NULL */
    TEST_ASSERT(icp2 != NULL);

    List* mergedCliques = malloc(sizeof(List));
    List_Init(mergedCliques);

    CliqueGroup_MergeCliques(mergedCliques, *icp1->clique,*icp2->clique, NULL/*doesn't matter in this test*/); //merge cliques frees no memory

    /* test and see if new clique has both items */
    //TODO


    /* free memory */
    List_Destroy(mergedCliques);
    free(mergedCliques);
    CliqueGroup_FreeValues(cg, Item_Free);
    CliqueGroup_Destroy(cg);
}

TEST_LIST = {
    { "CliqueGroup_test_add",   CliqueGroup_Test_Add },
    { "CliqueGroup_test_merge_cliques", CliqueGroup_Test_Merge_Cliques },
    { NULL, NULL }
};