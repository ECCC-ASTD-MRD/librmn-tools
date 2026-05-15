/*
 * Example libyaml parser.
 *
 * This is a simple libyaml parser example which scans and prints
 * the libyaml parser events.
 *
 */
#include <yaml.h>
void yaml_free(void *) ;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define STRVAL(x) ((x) ? (char*)(x) : "")

#define YAML_KEY_VALUE_EVENT 32

static yaml_event_t null_event = {.type        = YAML_NO_EVENT,
                                  .data.scalar = {NULL, NULL, NULL, 0L, 0, 0, 0},
                                  .start_mark  = {0L, 0L, 0L},
                                  .end_mark    = {0L, 0L, 0L} } ;

#define YAML_STACK_SIZE 128
typedef struct{                           // parser/analyzer stack
  yaml_char_t *anchor[YAML_STACK_SIZE] ;  // anchor name
  yaml_char_t *symbol[YAML_STACK_SIZE] ;  // part of data element name associated with this level
  yaml_char_t *value[YAML_STACK_SIZE] ;   // value of data element at this level
  yaml_char_t *tag[YAML_STACK_SIZE] ;     // tag of data element at this level
  int level ;                             // stack pointer, identation level
  int event_no[YAML_STACK_SIZE] ;         // event number in event list (only useful if anchor)
  char mode[YAML_STACK_SIZE] ;            // M if in mapping mode, S if in sequence mode
} yaml_stack_t ;

#define YAML_MAX_ALIASES 1024
typedef struct{
  int   na ;                              // number of aliases in table
  int   head[YAML_MAX_ALIASES] ;          // first event in this alias
  int   tail[YAML_MAX_ALIASES] ;          // last event in this alias
  yaml_char_t *name[YAML_MAX_ALIASES] ;   // anchor (name of this alias)
  yaml_char_t *value[YAML_MAX_ALIASES] ;  // value if scalar alias
  yaml_char_t *tag[YAML_MAX_ALIASES] ;    // tag if scalar alias
} yaml_alias_t ;

// information used for parsing
typedef struct{
  yaml_stack_t stack ;                    // mode stack
  yaml_alias_t aliases ;                  // aliases table (anchor values)
}yaml_context_t ;

yaml_char_t *yaml_alias_range(int *index, int *head, int *tail, yaml_alias_t *yaml_aliases, yaml_char_t *anchor){
  yaml_char_t *target = anchor ;
  *head = *tail = *index = -1 ;
  for(int i = 0 ; i < yaml_aliases->na ; i++){
    if( 0 == strncmp((char *)target, (char *)yaml_aliases->name[i], 1024) ){
      *head = yaml_aliases->head[i] ;
      *tail = yaml_aliases->tail[i] ;
      *index = i ;
      return yaml_aliases->value[i] ;
    }
  }
  return NULL ;  // not found or no scalar value associated with alias
}

// initialize parsing stack
void yaml_stack_init(yaml_stack_t *stack){
  stack->level = 0 ;                         // stack is empty
  for(int i=0 ; i<YAML_STACK_SIZE ; i++){
    stack->event_no[i] = -1 ;                // invalid event number
    stack->mode[i] = ' ' ;                   // invalid mode
    stack->anchor[i] = NULL ;                // no anchor
    stack->symbol[i] = NULL ;                // no symbol
    stack->value[i]  = NULL ;                // no value
    stack->tag[i]    = NULL ;                // no tag
  }
}

// value should be 'M', 'S', '{', or '(' for now
// '{', or '(' is cosmetic
void yaml_stack_push(yaml_stack_t *stack, char value){
  stack->level = stack->level + 1 ;
  stack->mode[stack->level] = value ;
}
char yaml_stack_pop(yaml_stack_t *stack){
  char r = stack->mode[stack->level] ;
  stack->level = stack->level - 1 ;
  return r ;
}
char yaml_get_stack_top(yaml_stack_t *stack){
  return stack->mode[stack->level] ;
}
// used to force stack top ot '}' or ')'  (cosmetic)
void yaml_set_stack_top(yaml_stack_t *stack, char value){
  stack->mode[stack->level] = value ;
}

#define YAML_STRMOV(DST,SRC) { DST = SRC ; SRC = NULL ; }

YAML_DECLARE(yaml_char_t *) yaml_strdup(const yaml_char_t *);
YAML_DECLARE(void) yaml_free(void *ptr);
YAML_DECLARE(void *) yaml_malloc(size_t size);

static FILE *parsed_out = NULL ;

static char _spaces[] = "                                                 " ;
char *spaces(int n){
  if(n <= 0) return "" ;
  return  _spaces + (sizeof(_spaces) - n - 1) ;
}

// print event description header
// stack    [IN] : stack pointer
// old_tos  [IN] : old top of stack (before this event)
// depth    [IN] : indentation level (number of spaces at end of header)
// event_id [IN] : possibly modified event number
static void print_header(yaml_stack_t *stack, char old_tos, int depth, int event_id)
{
    char tos = yaml_get_stack_top(stack) ;
    if(event_id < 0){                      // event inserted during alias processing
      printf("->%4.4d ", -event_id) ;
    }else{                                 // normal event
      printf("  %4.4d ", event_id) ;
    }
    printf("%c->%c|%2.2d|:[%2d]",old_tos, tos, stack->level, depth);
    printf("%s", spaces(depth)) ;
}

//TODO add tag to the fray
void print_parsed(yaml_stack_t *stack, FILE *f, int depth, char *key, char *value, char *tag){
  if(yaml_get_stack_top(stack) == 'S'){
    fprintf(f, "%s%s%s\n", spaces((depth-3)*2), key[0] ? "- " : "-", key) ;
    return ;
  }
  if(yaml_get_stack_top(stack) == 'M'){
    if(strncmp(key, "<<", 2) == 0) return ;   // ignore << alias insertion marker
    fprintf(f, "%s%s:", spaces((depth-3)*2), key) ;   // key:
    if(value[0]){
      if(tag[0]) fprintf(f, " %s", tag) ;             // tag if present
      fprintf(f, " %s", value) ;                      // value
    }
    fprintf(f, "\n") ;
//     fprintf(f, "%s%s:%s%s\n", spaces((depth-3)*2), key, value[0] ? " " : "", value) ;
    return ;
  }
}

void print_symbol_stack(yaml_stack_t *stack, FILE *f){
  int i ;
  char *has_no_value = ".key_only=1" ;
  char dot = ' ' ;
  char printed = 0 ;

  // NO-OP if any token is "<<"
  for(i = 0 ; i <= stack->level ; i++){
    char *symbol = (char *)stack->symbol[i] ;
    if(symbol == NULL) continue ;
    if(symbol[0] == 0) continue ;
    if( strncmp(symbol, "<<", 2) == 0 ) return ;
  }

  for(i = 0 ; i <= stack->level ; i++){
    has_no_value = "key_only=1" ;
    has_no_value = "" ;
    char *symbol = (char *)stack->symbol[i] ;

    if(symbol == NULL) continue ;
    if(symbol[0] == 0) continue ;

    if(! printed) fprintf(f, "#      ") ;
    printed = 1 ;
    if(stack->value[i] != NULL){
      char *value = (char *)stack->value[i] ;
      char *tag = stack->tag[i] ? (char *)stack->tag[i] : "" ;
      if( value[0] == '\0' ) value = "' '" ;
      fprintf(f, "%c%s = %s %s", dot, symbol, tag, value) ;
//       fprintf(f, "") ;
      has_no_value = "" ;
    }else{
      fprintf(f, "%c%s", dot, symbol) ;
    }
    dot = '.' ;
  }
//   if(! printed) has_no_value = "#" ;
  if(printed)fprintf(f, "%s\n", has_no_value) ;
}

// initialize aliases table
void yaml_aliases_init(yaml_alias_t *table){
  table->na = 0 ;                              // no valid aliases
  for(int i=0 ; i<YAML_MAX_ALIASES ; i++){
    table->head[i]  = 0 ;                      // no start of anchor
    table->tail[i]  = 0 ;                      // no end of anchor
    table->name[i]  = NULL ;                   // no name
    table->value[i] = NULL ;                   // no value
    table->tag[i]   = NULL ;                   // no tag
  }
}

void yaml_context_init(yaml_context_t *context){
  yaml_stack_init(&(context->stack)) ;               // initialize the stack
  yaml_aliases_init(&(context->aliases)) ;           // initialize the aliases table
}

#define CHECK_YAML_STACK if(level != stack->level) exit(1)
#define PRINT_INDENT(SPACES) print_header(stack, tos, SPACES, event_id)
// context [INOUT] : information used for parsing
// event_no   [IN] : index in list of event being processed
// event_id   [IN] : pseudo event number, only used in diagnostic prints
// list       [IN] : event list (array of events)
int process_event(yaml_context_t *context, int event_no, int event_id, yaml_event_t *list)
{
    yaml_alias_t *aliases = &(context->aliases) ;   // anchor name and position table
    yaml_stack_t *stack = &(context->stack) ;       // mode stack (sequence | mapping | stream | document)
    char tos = yaml_get_stack_top(stack) ;          // current top of mode stack (only used in diagnostic prints)
    yaml_event_t *event = &list[event_no] ;                                      // event being processed
    yaml_event_t *next_event = event + 1 ;                                       // following event
    yaml_event_t *prev_event = (event_no == 0) ? (&null_event) : (event - 1) ;   // preceding event
    int level = stack->level ;                      // current level in mode stack
    int print_symbols = 0 ;
    int head = -1, tail = -1, index = -1 ;

    CHECK_YAML_STACK ;
    switch ((int)event->type) {
    case YAML_NO_EVENT:
//         PRINT_INDENT(level);
//         printf("NO-OP (%d)\n", event->type);
        CHECK_YAML_STACK ;
        break;

    case YAML_STREAM_START_EVENT:
        CHECK_YAML_STACK ;
        yaml_stack_push(stack, '{') ; level++ ;
        PRINT_INDENT(level - 1);
        printf("STREAM_START (%d)\n", event->type);
        yaml_set_stack_top(stack, '}') ;
        CHECK_YAML_STACK ;
        break;

    case YAML_STREAM_END_EVENT:
        CHECK_YAML_STACK ;
        yaml_stack_pop(stack); level-- ;
        PRINT_INDENT(level);
        printf("STREAM_END (%d)\n", event->type);
        CHECK_YAML_STACK ;
        break;

    case YAML_DOCUMENT_START_EVENT:
        CHECK_YAML_STACK ;
        yaml_stack_push(stack, '(') ; level++ ;
        PRINT_INDENT(level - 1);
        printf("DOC_START (%d)\n", event->type);
        yaml_set_stack_top(stack, ')') ;
        CHECK_YAML_STACK ;
        break;

    case YAML_DOCUMENT_END_EVENT:
        CHECK_YAML_STACK ;
        yaml_stack_pop(stack); level-- ;
        PRINT_INDENT(level);
        printf("DOC_END (%d)\n", event->type);
        CHECK_YAML_STACK ;
        break;

    case YAML_ALIAS_EVENT:
        CHECK_YAML_STACK ;
        PRINT_INDENT(level);
        yaml_alias_range(&index, &head, &tail, aliases, event->data.alias.anchor) ;
        printf("ALIAS[%d] (%d), {anchor=\"%s\"}, events[%4.4d:%4.4d]\n",
               event_id,
               event->type,
               STRVAL(event->data.alias.anchor),
               head, tail
              );
        // if alias was a scalar alias (head == tail), alias event has been replaced with key/value event
        // key from previous event, value from alias table
        if(tail == head){
          PRINT_INDENT(level);
          printf("ERROR: SCALAR alias encountered\n");
        }else{
          // if preceded by <<:, eliminate first and last event in what anchor points to
          if(prev_event->type == YAML_SCALAR_EVENT){
            if( strncmp((char *)prev_event->data.scalar.value, "<<", 2) == 0 ){
              head++ ;
              tail-- ;
            }
          }
          for(int i = head ; i <= tail ; i++){    // inject what anchor points to
            process_event(context, i, -i, list) ; // head <= event number <= tail
          }
        }
        CHECK_YAML_STACK ;
        break;

    case YAML_KEY_VALUE_EVENT:
        CHECK_YAML_STACK ;
        PRINT_INDENT(level);
        printf("KEY_VALUE[%d] (%d) = {key=\"%s\", value=\"%s\", length=%d}, tag=\"%s\"\n",
              event_id,
              event->type,
              STRVAL(event->data.scalar.anchor),
              STRVAL(event->data.scalar.value),
              (int)event->data.scalar.length,
              STRVAL(event->data.scalar.tag)  );
        print_parsed(stack, parsed_out, level,
                     STRVAL(event->data.scalar.anchor),
                     STRVAL(event->data.scalar.value),
                     STRVAL(event->data.scalar.tag)) ;
        // insert key=value as symbol[level]
        stack->symbol[level] = yaml_strdup( event->data.scalar.anchor ) ;
        stack->value[level] = yaml_strdup( event->data.scalar.value ) ;
        stack->tag[level] = yaml_strdup( event->data.scalar.tag ) ;
        stack->symbol[level+1] = stack->value[level+1] = stack->tag[level+1] = NULL ;
        print_symbols = 1 ;
        CHECK_YAML_STACK ;
        break;

    case YAML_SCALAR_EVENT:
        CHECK_YAML_STACK ;
        // if next event is a scalar ALIAS
        // next event becomes a scalar event, value from alias table
        if(next_event->type == YAML_ALIAS_EVENT)
        {
          PRINT_INDENT(level);
          char *value = (char *)yaml_alias_range(&index, &head, &tail, aliases, next_event->data.alias.anchor) ;
          printf("SCALAR_ALIAS '|%s|%s|' FOLLOWS, head,tail = %d,%d, entry = '|%s|%s|%s|'\n",
                 next_event->data.alias.anchor,
                 value ? value : "NoNe",
                 head, tail, aliases->name[index], aliases->value[index], aliases->tag[index] ) ;
          if(value)    // there is a value associated with the alias in next event
          {
            yaml_event_t et ;
            et.type = YAML_SCALAR_EVENT ;
            et.data.scalar.value = (yaml_char_t *)value ;
            et.data.scalar.length = strnlen(value, 1024) ;
            et.data.scalar.anchor = NULL ;
            et.data.scalar.tag = aliases->tag[index] ;
            et.data.scalar.plain_implicit = event->data.scalar.plain_implicit ;
            et.data.scalar.quoted_implicit = event->data.scalar.quoted_implicit ;
            *next_event = et ;  // alias -> scalar, value coming from alias table
          }
        }
        // SCALAR followed by event with ANCHOR, insert anchor into aliases table
        if(next_event->type == YAML_SCALAR_EVENT &&
           next_event->data.scalar.anchor != NULL)
        {
          aliases->name[aliases->na] = next_event->data.scalar.anchor ;
          aliases->value[aliases->na] = yaml_strdup(next_event->data.scalar.value) ;
          aliases->tag[aliases->na] = (yaml_char_t *)STRVAL(next_event->data.scalar.tag) ;
          next_event->data.scalar.anchor = NULL ;
          aliases->head[aliases->na] = event_no + 1 ;   // start point
          aliases->tail[aliases->na] = event_no + 1 ;   // end = start for scalar
          PRINT_INDENT(level);
          printf("SCALAR_ANCHOR = '%s|%s|%s' at %d\n",
                 aliases->name[aliases->na],
                 aliases->value[aliases->na],
                 aliases->tag[aliases->na],
                 event_no + 1) ;
          aliases->na = aliases->na + 1 ;
        }
        // should next event be transformed into key = value ?
        if(next_event->type == YAML_SCALAR_EVENT   &&     // next event is SCALAR
           yaml_get_stack_top(stack) == 'M' &&            // we are in mapping mode
           next_event->data.scalar.anchor == NULL)        // next event is not an anchor
        {
          next_event->data.scalar.anchor = event->data.scalar.value ;  // current value becomes key for next event
          event->data.scalar.value = NULL ;           // make sure value string does not get freed when disposing of event
          next_event->type = YAML_KEY_VALUE_EVENT ;   // next event is key = value type (fudged YAML event type)
          event->type = YAML_NO_EVENT ;               // make current event a NO-OP
          CHECK_YAML_STACK ;
          break;                                      // do not print nor parse, we are done
        }
        PRINT_INDENT(level);
        printf("SCALAR[%d] (%d) = {value=\"%s\", length=%d}, anchor=\"%s\", tag=\"%s\"\n",
              event_id,
              event->type,
              STRVAL(event->data.scalar.value),
              (int)event->data.scalar.length,
              STRVAL(event->data.scalar.anchor),
              STRVAL(event->data.scalar.tag)  );
        if(next_event->type != YAML_MAPPING_START_EVENT &&
           next_event->type != YAML_ALIAS_EVENT)
        {  // scalar not followed by mapping or alias
          print_parsed(stack, parsed_out, level,
                       STRVAL(event->data.scalar.value),
                       "",
                       STRVAL(event->data.scalar.tag)) ;  // single value
          // insert value as symbol[level]
          stack->symbol[level] = yaml_strdup( event->data.scalar.value ) ;
          stack->value[level] = stack->tag[level] = NULL ;
          stack->symbol[level+1] = stack->value[level+1] = stack->tag[level+1] = NULL ;
          print_symbols = 1 ;
        }
        CHECK_YAML_STACK ;
        break;

    case YAML_SEQUENCE_START_EVENT:
        CHECK_YAML_STACK ;
        yaml_stack_push(stack, 'S') ; level++ ;
        PRINT_INDENT(level - 1);
        if(event->data.sequence_start.anchor){
          stack->anchor[level] = event->data.sequence_start.anchor ;
          event->data.sequence_start.anchor = NULL ;
          stack->event_no[level] = event_no ;           // anchor range starts here
          printf("SEQUENCE_ANCHOR_START(%d) : anchor='%s' at event %d\n",
                 level,
                 stack->anchor[level],
                 stack->event_no[level]) ;
        }else{
          printf("SEQUENCE_START[%d] (%d), tag=\"%s\"\n",
                event_id,
                event->type,
                STRVAL(event->data.sequence_start.tag));
        }
        CHECK_YAML_STACK ;
        break;

    case YAML_SEQUENCE_END_EVENT:
        CHECK_YAML_STACK ;
        PRINT_INDENT(level - 1);
        if( stack->anchor[level] ){
          printf("SEQUENCE_ANCHOR_END(%d) : '%s', events %d to %d\n",
                 level, stack->anchor[level], stack->event_no[level], event_id) ;
          aliases->name[aliases->na] = stack->anchor[level] ;
          aliases->value[aliases->na] = NULL ;                        // not a scalar anchor
          aliases->tag[aliases->na] = NULL ;                          // not a scalar anchor
          stack->anchor[level] = NULL ;
          aliases->head[aliases->na] = stack->event_no[level] ;       // remembered start point
          aliases->tail[aliases->na] = event_no ;                     // anchor range ends here
          aliases->na = aliases->na + 1 ;
        }else{
          printf("SEQUENCE_END[%d] (%d)\n",
                event_id,
                event->type);
        }
        yaml_stack_pop(stack); level-- ;
        CHECK_YAML_STACK ;
        break;

    case YAML_MAPPING_START_EVENT:
        CHECK_YAML_STACK ;
        PRINT_INDENT(level);
        char *name="" ;
        // if previous event was SCALAR, mapping name comes from its value
        if(prev_event->type == YAML_SCALAR_EVENT) name = (char *)prev_event->data.scalar.value ;
        name = name ? name : "NULL" ;
        if(event->data.mapping_start.anchor){
          print_parsed(stack, parsed_out, level, name, "", "") ;
          yaml_stack_push(stack, 'M') ; level++ ;
          stack->anchor[level] = event->data.mapping_start.anchor ;
          stack->symbol[level-1] = yaml_strdup( (yaml_char_t *)name ) ;
          stack->value[level-1] = stack->tag[level-1] = NULL ;
          stack->symbol[level] = stack->value[level] = stack->tag[level] = NULL ;
          event->data.mapping_start.anchor = NULL ;
          stack->event_no[level] = event_no ;     // anchor range starts at this event
          printf("MAPPING_ANCHOR_START(%d) : name='%s', anchor='%s' at event %d\n",
                 level,
                 name,
                 stack->anchor[level],
                 stack->event_no[level]) ;
        }else{
          printf("MAPPING_START[%d] (%d), name='%s', anchor=\"%s\", tag=\"%s\"\n",
                event_id,
                event->type,
                 name,
                STRVAL(event->data.mapping_start.anchor),
                STRVAL(event->data.mapping_start.tag));
          print_parsed(stack, parsed_out, level, name, "", "") ;
          stack->symbol[level] = yaml_strdup( (yaml_char_t *)name ) ;
          stack->value[level] = stack->tag[level] = NULL ;
          stack->symbol[level+1] = stack->value[level+1] = stack->tag[level+1] = NULL ;
          yaml_stack_push(stack, 'M') ; level++ ;
        }
        print_symbols = 1 ;
        CHECK_YAML_STACK ;
        break;

    case YAML_MAPPING_END_EVENT:
        CHECK_YAML_STACK ;
        PRINT_INDENT(level - 1);
        if( stack->anchor[level] ){
          printf("MAPPING_ANCHOR_END(%d) : '%s', events %d to %d\n",
                 level, stack->anchor[level],
                 stack->event_no[level],
                 event_no - 1) ;
//           aliases->name[aliases->na] = (char *)yaml_strdup(stack->anchor[level]) ;
//           free(stack->anchor[level]) ;
          aliases->name[aliases->na] = stack->anchor[level] ;
          aliases->value[aliases->na] = NULL ;                        // not a scalar anchor
          aliases->tag[aliases->na] = NULL ;                          // not a scalar anchor
          stack->anchor[level] = NULL ;
          aliases->head[aliases->na] = stack->event_no[level] ;       // remembered start point
          aliases->tail[aliases->na] = event_no ;                     // anchor range ends here
          aliases->na = aliases->na + 1 ;
        }else{
          printf("MAPPING_END[%d] (%d)\n",
                event_id,
                event->type);
        }
        yaml_stack_pop(stack); level-- ;
        CHECK_YAML_STACK ;
        break;

    default:
        printf("OTHER (%d)\n", event->type);
        CHECK_YAML_STACK ;
        break;
    }

    if (level < 0) {
        printf("ERROR: indentation underflow!\n");
        return 1 ;
    }
    if(print_symbols) print_symbol_stack(stack, parsed_out) ;
    return 0 ;
}

int parse_event_list(yaml_context_t *context, yaml_event_t *list){
  yaml_event_t event ;
  yaml_event_type_t event_type ;
  int event_no = 0 ;

  do{
    event = list[event_no] ;
    event_type = event.type ;
    if( process_event(context, event_no, event_no, list) ) return 0 ;
    event_no++ ;
  }while (event_type != YAML_STREAM_END_EVENT) ;

  return (event_type == YAML_STREAM_END_EVENT) ;
}


#define MAX_EVENTS 1024

int main(int argc, char *argv[])
{
    yaml_event_t *events = (yaml_event_t *)malloc((MAX_EVENTS+2) * sizeof(yaml_event_t)) ;
    int event_no ;
    char *filename = (argc > 1) ? argv[1] : "stderr" ;

    yaml_context_t yaml_context, *yaml_context_p = &yaml_context ;
    yaml_alias_t *aliases = &(yaml_context.aliases) ;

    yaml_parser_t parser;
    yaml_event_type_t event_type;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, stdin);

    if(argc > 1){
      parsed_out = fopen(filename, "w+") ;
      if(parsed_out == NULL) exit(1) ;
    }else{
      parsed_out = stderr ;
    }
    fprintf(parsed_out, "---\n") ;

    // build event list
    for(event_no=0 ; event_no<MAX_EVENTS ; event_no++) {
        if( !yaml_parser_parse(&parser, &events[event_no]) )
            goto error ;
        event_type = events[event_no].type;
        if(event_type == YAML_STREAM_END_EVENT) break ;    // end of steam
    } ;
    if(event_type != YAML_STREAM_END_EVENT) goto error ;   // event list was too small
    events[event_no+1] = null_event ;

    // parse event list
    yaml_context_init(yaml_context_p) ;
    if( !parse_event_list(yaml_context_p, events) ) goto error2 ;

    yaml_parser_delete(&parser);

    printf("'%s' used for parsed YAML output\n", filename) ;
    printf("event_no = %d\n", event_no) ;
    printf("sizeof(yaml_event_t) = %ld\n", sizeof(yaml_event_t)) ;
    fprintf(parsed_out, "...\n") ;
    fclose(parsed_out) ;
    printf("alias table\n") ;
    for(int i=0 ; i<aliases->na ; i++){
      printf("%3d : [%4.4d:%4.4d] '%s', value='%s', tag='%s'\n", i, aliases->head[i], aliases->tail[i],  aliases->value[i],  aliases->name[i], aliases->tag[i]) ;
    }
    return EXIT_SUCCESS;

error:
    printf("ERROR: Failed to parse: %s\n", parser.problem);
    yaml_parser_delete(&parser);
    return EXIT_FAILURE;

error2:
    printf("ERROR: analyzing event list\n") ;
    return EXIT_FAILURE;
}
