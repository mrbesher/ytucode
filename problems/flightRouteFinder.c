#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CITY_CHAR_LIMIT 25
#define INIT_ARR_SIZE 2

typedef struct {
    char src[CITY_CHAR_LIMIT];
    char dest[CITY_CHAR_LIMIT];
    int hours;
    int mins;
    int price;
} Flight;

typedef struct {
    int price;
    int duration;
} Cost;

typedef struct {
    char **mappings;
    Cost **adjMatrix;
    size_t nmemb; /* number of cities in the graph */
} Graph;

typedef struct {
    int src;
    int dest;
    int (*compar)(Cost *, Cost *);
    size_t transitlimit;
} Preference;

typedef struct _Citynode {
    int cityId;
    struct _Citynode *next;
} Citynode;

typedef struct _Path {
    Cost *cost;
    Citynode *head;
    struct _Path *next;
} Path;

/* GRAPH FUNCTIONS */

Graph *create_flights_graph(Flight *, size_t);
char **create_mappings(Flight *, size_t, size_t *);
size_t get_city_id(char *, char **, size_t);

/* PATH FUNCTIONS */

Path get_all_paths_sorted(Graph *, Preference);
bool is_city_in_path(int, Citynode *);
int add_full_path(Path **, Citynode *, Graph *, Preference);
int add_all_paths(Graph *, Path **, Preference);
int add_paths_from_node(Graph *, Path **, Citynode **, int, Preference, size_t);

int compar_cost_price(Cost *, Cost *);
int compar_cost_dur(Cost *, Cost *);

/* STACK FUNCTONS */

int remove_head(Citynode **);
int add_city_to_stack(Citynode **, int);

/* IO FUNCTIONS */

Flight *read_flights_file(const char *, size_t *);
Preference get_pref_usr(Graph *);

/* MEMORY FUNCTIONS */

void free_graph(Graph *);

/* GENERIC FUNCTIONS */

bool is_str_in_arr(char[], char **, size_t);

int main(int argc, char const *argv[]) {
    Flight *flights;    /* An array containing flights read from user */
    Graph *graph;       /* A graph containing all connections and ids of cities */
    Preference usrPref; /* user choices for the flight */
    Path *paths = NULL;
    size_t inputsize, i, j;

    if (argc > 1) {
        flights = read_flights_file(argv[1], &inputsize);
    } else {
        printf("Error: No argument provided. Run like this\n./flightRouteFinder flights.txt\n");
        return 1;
    }

    if (!flights) {
        printf("%d > Error: Couldn't create / read flights.", __LINE__);
        return 1;
    }

    printf("src des hr mi price\n");
    for (i = 0; i < inputsize; i++) {
        printf("%s %s %2d %2d %4d\n", flights[i].src, flights[i].dest,
               flights[i].hours, flights[i].mins, flights[i].price);
    }

    graph = create_flights_graph(flights, inputsize);
    if (!graph) {
        printf("%d > Error: Couldn't create graph.", __LINE__);
        free(flights);
        return 1;
    }

    printf("\n\n");
    for (i = 0; i < graph->nmemb; i++) {
        for (j = 0; j < graph->nmemb; j++) {
            printf("%4d\t", graph->adjMatrix[i][j].duration);
        }
        printf("\n");
    }

    for (i = 0; i < graph->nmemb; i++)
        printf("%ld: %s\n", i, graph->mappings[i]);
    printf("\n\n");

    usrPref = get_pref_usr(graph);
    /*
    paths = get_all_paths_sorted(graph, usrPref);
    */

    add_all_paths(graph, &paths, usrPref);

    for (Path *p = paths; p; p = p->next) {
        printf("%d: ", p->cost->duration);
        for (Citynode *c = p->head; c; c = c->next)
            printf("%s  ", graph->mappings[c->cityId]);
        printf("\n");
    }

    free(flights);
    free_graph(graph);
    return 0;
}

/* GRAPH TRAVERSE FUNCTIONS */
int add_all_paths(Graph *graph, Path **paths, Preference pref) {
    Citynode *visited = NULL;
    int srcId = pref.src;
    int destId = pref.dest;
    add_city_to_stack(&visited, srcId);
    return add_paths_from_node(graph, paths, &visited, destId, pref, 0);
}

int add_paths_from_node(Graph *graph, Path **paths, Citynode **visited, int destId, Preference pref, size_t depth) {
    Citynode *currCity = *visited;
    size_t i;
    Cost connCost;

    /* reached limit */
    if (depth > pref.transitlimit) {
        remove_head(visited);
        return 0;
    }

    for (i = 0; i < graph->nmemb; i++) {
        connCost = graph->adjMatrix[currCity->cityId][i];
        /* if a flight exists between current city and dest */
        if (i == (size_t)destId && connCost.price != 0) {
            printf("Found dest from %s to %s\n", graph->mappings[currCity->cityId], graph->mappings[destId]);
            add_city_to_stack(visited, i);
            add_full_path(paths, *visited, graph, pref);
            remove_head(visited);
        } else if (connCost.price != 0 && !is_city_in_path(i, *visited)) {
            /* if city has a connection and not visited */
            printf("Visiting %s\n", graph->mappings[i]);
            add_city_to_stack(visited, i);
            add_paths_from_node(graph, paths, visited, destId, pref, depth + 1);
        }
    }
    /* pop self from stack */
    printf("backtracking from %s\n", graph->mappings[(*visited)->cityId]);
    remove_head(visited);
    return 0;

    /*
    currCity = get_head(visited);
    for neighbor in currCity.neighbors:
        if neighbor == destId:
            break;
        if not visited:
            add_to_visited(visited, currCity);
            add_paths_from_node(graph, paths, visited, destId);
    if neighbor == destId:
        add_path(Paths, visited)
    reutrn 0;
    */
}

int add_full_path(Path **paths, Citynode *visited, Graph *graph, Preference pref) {
    Path *temppaths = *paths;
    Citynode *tempcity = visited;
    Citynode *newcity;
    Path *currPath;
    Cost *cost, tempcost;
    printf("%d> CALLED\n",__LINE__);
    currPath = malloc(sizeof(Path));
    cost = malloc(sizeof(Cost));
    cost->duration = 0;
    cost->price = 0;
    currPath->cost = cost;

    newcity = (Citynode *)malloc(sizeof(Citynode));
    newcity->cityId = tempcity->cityId;
    newcity->next = NULL;
    currPath->head = newcity;

    /* add cities to currPath */
    while ((tempcity = tempcity->next)) {
        /* add current link cost */
        tempcost = graph->adjMatrix[newcity->cityId][tempcity->cityId];
        currPath->cost->price += tempcost.price;
        currPath->cost->duration += tempcost.duration + 1;

        /* add new node to path */
        newcity = (Citynode *)malloc(sizeof(Citynode));
        newcity->cityId = tempcity->cityId;
        newcity->next = currPath->head;
        currPath->head = newcity;
    }

    /*
    number of transits is `nodes - 2`
    one city is not counted above already
    */
    (currPath->cost->duration)--;

    /* linkedlist empty */
    if (!temppaths) {
        *paths = currPath;
        currPath->next = NULL;
        return 0;
    }

    /* path has only one node */
    if (!(temppaths->next)) {
        /* if cost of current path is more */
        if (pref.compar(temppaths->cost, currPath->cost) > 0) {
            currPath->next = temppaths;
        } else {
            temppaths->next = currPath;
            currPath->next = NULL;
        }
        *paths = currPath;
        return 0;
    }

    /* calculated path has less cost than the head path */
    if (pref.compar(temppaths->cost, currPath->cost) > 0) {
        currPath->next = temppaths;
        *paths = currPath;
        return 0;
    }

    while (temppaths->next && pref.compar(temppaths->next->cost, currPath->cost) < 0) {
        temppaths = temppaths->next;
    }

    currPath->next = temppaths->next;
    temppaths->next = currPath;
    return 0;
}

int add_city_to_stack(Citynode **visited, int id) {
    Citynode *city = malloc(sizeof(Citynode));
    if (!city)
        return -1;

    /* intialize Citynode */
    city->cityId = id;
    city->next = *visited;
    *visited = city;

    return 0;
}

int remove_head(Citynode **path) {
    Citynode *temp = *path;
    if (!temp)
        return -1;

    (*path) = (*path)->next;
    free(temp);
    return 0;
}

/* GRAPH STRUCTURAL FUNCTIONS */

Graph *create_flights_graph(Flight *flights, size_t inputsize) {
    size_t i;
    size_t row, col;
    Graph *graph;

    graph = (Graph *)malloc(sizeof(Graph));

    graph->mappings = create_mappings(flights, inputsize, &(graph->nmemb));

    graph->adjMatrix = (Cost **)malloc(graph->nmemb * sizeof(Cost *));
    /* set all costs to 0 indicating no flight and allocate memory */
    for (i = 0; i < graph->nmemb; i++)
        graph->adjMatrix[i] = (Cost *)calloc((graph->nmemb), sizeof(Cost));

    for (i = 0; i < inputsize; i++) {
        row = get_city_id(flights[i].src, graph->mappings, graph->nmemb);
        col = get_city_id(flights[i].dest, graph->mappings, graph->nmemb);
        if (row == UINT_MAX || col == UINT_MAX) {
            free_graph(graph);
            printf("%d > [FATAL ERROR] City name not found. Although just inserted!\n", __LINE__);
            return NULL;
        }
        graph->adjMatrix[row][col].duration = flights[i].hours * 60 + flights[i].mins;
        graph->adjMatrix[row][col].price = flights[i].price;
        graph->adjMatrix[col][row] = graph->adjMatrix[row][col];
    }
    return graph;
}

/*
* returns the id of the city string
* returns UINT_MAX on failure
*/
size_t get_city_id(char *city, char **cities, size_t nmemb) {
    size_t i;
    for (i = 0; i < nmemb; i++) {
        if (!strcmp(city, cities[i])) {
            return i;
        }
    }
    return UINT_MAX;
}

/* returns true if `cityId` is in path */
bool is_city_in_path(int keyCity, Citynode *path) {
    Citynode *temp = path;
    while (temp->next && temp->cityId != keyCity)
        temp = temp->next;

    if (temp->cityId == keyCity)
        return true;

    return false;
}

char **create_mappings(Flight *flights, size_t inputsize, size_t *nmemb) {
    size_t i, arrSize;
    char **mappings;

    arrSize = INIT_ARR_SIZE;
    mappings = (char **)malloc(arrSize * sizeof(char *));
    *nmemb = 0;

    for (i = 0; i < inputsize; i++) {
        /* if flights[i].src not in mappings add it */
        if (!is_str_in_arr(flights[i].src, mappings, *nmemb)) {
            mappings[*nmemb] = (char *)malloc(CITY_CHAR_LIMIT * sizeof(char));
            strcpy(mappings[(*nmemb)++], flights[i].src);
        }

        /* if flights[i].dest not in mappings add it */
        if (!is_str_in_arr(flights[i].dest, mappings, *nmemb)) {
            mappings[*nmemb] = (char *)malloc(CITY_CHAR_LIMIT * sizeof(char));
            strcpy(mappings[(*nmemb)++], flights[i].dest);
        }

        if (*nmemb + 2 > arrSize) {
            arrSize <<= 1;
            mappings = (char **)realloc(mappings, arrSize * sizeof(char *));
        }
    }

    return mappings;
}

int compar_cost_price(Cost *a, Cost *b) {
    return a->price - b->price;
}

int compar_cost_dur(Cost *a, Cost *b) {
    return a->duration - b->duration;
}

/* GENERIC FUNCTIONS */

bool is_str_in_arr(char key[], char **strarr, size_t nmemb) {
    size_t i;
    for (i = 0; i < nmemb; i++) {
        if (!strcmp(key, strarr[i]))
            return true;
    }

    return false;
}

/* IO FUNCTIONS */

Flight *read_flights_file(const char *filename, size_t *size) {
    FILE *fp;
    Flight *flights;
    size_t i = 0;
    size_t j = INIT_ARR_SIZE; /* ads array size */

    if (!(fp = fopen(filename, "rb"))) {
        printf("Couldn't read %s\nProbably doesn't exist\n", filename);
        return NULL;
    }

    printf("\n> Processing file %s\n", filename);
    flights = (Flight *)malloc(j * sizeof(Flight));
    while (fscanf(fp, " %s %s %d %d %d", flights[i].src, flights[i].dest, &flights[i].hours,
                  &flights[i].mins, &flights[i].price) == 5) {
        i++;
        if (i >= j - 1) {
            j <<= 1;
            flights = realloc(flights, j * sizeof(Flight));
        }
    }
    fclose(fp);
    *size = i;
    return flights;
}

Preference get_pref_usr(Graph *g) {
    Preference pref;
    char ans[CITY_CHAR_LIMIT];
    /* get departure city and remove trailing white space */
    printf("Departure city: ");
    fgets(ans, CITY_CHAR_LIMIT, stdin);
    strtok(ans, "\n");
    strtok(ans, "\r");
    pref.src = get_city_id(ans, g->mappings, g->nmemb);

    /* get destination city and remove trailing white space */
    printf("Destination: ");
    fgets(ans, CITY_CHAR_LIMIT, stdin);
    strtok(ans, "\n");
    strtok(ans, "\r");
    pref.dest = get_city_id(ans, g->mappings, g->nmemb);

    printf("Max # of transits: ");
    scanf(" %lu", &pref.transitlimit);

    printf("(p)rice / (t)ime: ");
    scanf(" %s", ans);
    pref.compar = (ans[0] == 't' || ans[0] == 'T') ? compar_cost_dur : compar_cost_price;

    return pref;
}

/* memory functions */

void free_graph(Graph *a) {
    size_t i;

    for (i = 0; i < a->nmemb; i++)
        free(a->mappings[i]);

    free(a->mappings);
    /* free all matrices rows */
    for (i = 0; i < a->nmemb; i++)
        free(a->adjMatrix[i]);
    free(a->adjMatrix);
    free(a);
}