#include <gtk/gtk.h> // No es necesario incluir GLib ni nada más en particular, ya que GTK incluye
#include <vte/vte.h> // todas esas cosas.
#define Length(arr) (sizeof (arr) / sizeof ((arr)[0]))

/*
 * Preferiblemente, declaramos variables antes que cualquier cosa. Nótese que normalmente (o por lo
 * menos en muchos ejemplos de código) se usaría el tipo GtkWidget*, pero hacer eso significa que
 * debemos convertir cada variable al tipo requerido por sus métodos. Por ejemplo:
 *
 * GtkWidget *stack;
 * gtk_stack_set_visible_child_name((GtkStack*) stack, "name");
 *
 * Sería un fastidio tener que convertir cosas a cada rato, así que mejor simplemente usamos los tipos
 * de datos correspondientes y cuando se deba convertir algo a GtkWidget, se hace. Al final, todo esto
 * funciona gracias a que Gtk usa de cierta forma el paradigma de POO y muchos widgets heredan de
 * GtkWidget.
 */

GtkApplication   *app;
GtkBuilder       *builder;
GtkWindow        *window;
GtkMessageDialog *exitDialog;
GtkStack         *stack, *deskImgStack;
GtkButton        *backBtn, *nextBtn, *exitBtn;
GtkViewport      *termView;
GtkHeaderBar     *header;
GtkListBox       *deskImgSwitcher;
GtkListBoxRow    *deskRow1, *deskRow2, *deskRow3;
VteTerminal      *term;
VtePty           *pty;

// En este array de strings guardo los nombres de todas las páginas del stack principal
const gchar *stackPages[] = { "welcome", "language", "disk", "partitions", "user",
                              "desktop", "software", "extras", "terminal", "end" };

// Esta función se encarga de cambiar las páginas del stack principal. Ambos botones ("Atrás" y
// "Siguiente") usan esta función.
void mainPageSwitcher(GtkButton *btn, gpointer gparted) {
  const gchar *page    = gtk_stack_get_visible_child_name(stack); // La página visible actualmente
  const gchar *btnName = gtk_widget_get_name((GtkWidget*) btn);   // Nombre del botón clickeado
  gint iter = 0;

  for (iter; iter < Length(stackPages); iter++) {
    if (g_strcmp0(page, stackPages[iter]) == 0) {
      if (g_strcmp0(btnName, "backBtn") == 0) gtk_stack_set_visible_child_name(stack, stackPages[iter - 1]);
      if (g_strcmp0(btnName, "nextBtn") == 0) gtk_stack_set_visible_child_name(stack, stackPages[iter + 1]);
    }
  }
}

// En la página de selección de escritorio, esta función es utilizada para cambiar entre las previews
// que se muestran de los escritorios.
void deskImgOnSwitch(GtkListBox *box, GtkListBoxRow *row, gpointer data) {
  const gchar *rowName = gtk_widget_get_name((GtkWidget*) row); // Nombre de la celda seleccionada

  if (g_strcmp0(rowName, "desktopImgSwitch1") == 0) { gtk_stack_set_visible_child_name(deskImgStack, "desktopImg1");
  } else if (g_strcmp0(rowName, "desktopImgSwitch2") == 0) { gtk_stack_set_visible_child_name(deskImgStack, "desktopImg2");
  } else if (g_strcmp0(rowName, "desktopImgSwitch3") == 0) { gtk_stack_set_visible_child_name(deskImgStack, "desktopImg3"); }
}

// Esta función es llamada cuando el botón "Salir" es clickeado
void exitBtn_clicked(GtkButton *btn, gpointer data) {
  exitDialog = (GtkMessageDialog*) gtk_builder_get_object(builder, "exitDialog");
  int res = gtk_dialog_run((GtkDialog*) exitDialog);

  switch (res) {
    case GTK_RESPONSE_OK:
      g_application_quit((GApplication*) app);
      break;
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_CANCEL:
      gtk_widget_hide((GtkWidget*) exitDialog);
      break;
    default:
      g_print("No action for %d\n", res);
      break;
  }
}


// Con esta función, se abre la shell del usuario en la terminal de la pagina de la terminal
void spawnShell(VteTerminal *term, gint status, gpointer data) {
  // La documentación de VTE indica que este array debe ser "zero-terminated" y para poder abrir un
  // shell en la vista de VTE, se debe especificar en el primer índice. En realidad, se pueden pasar
  // otros argumentos, claro está. Pero en este caso necesitamos un shell.
  char **termArgv = (char *[]) { getenv("SHELL"), NULL };
  // Hay otras formas de hacer esto, pero debido a que solamente es un ejemplo, lo he hecho de esta forma.
  // La explicación es la siguiente:
  vte_terminal_spawn_async(
    term,            // Nuestro "objeto" VteTerminal*
    VTE_PTY_DEFAULT, // Banderas del Pty (ver VtePtyFlags en la documentación de VTE)
    NULL,            // Directorio de trabajo (NULL para usar el actual)
    termArgv,        // "Arguments Vector" (argv)
    NULL,            // "Environment Vector" (envv), esto lo podemos dejar en NULL para no modificar ninguna
                     // variable de entorno.
    G_SPAWN_DEFAULT, // Banderas de spawneo (ver GSpawnFlags en la documentación de GLib)
    NULL,            // "child_setup", sinceramente, no sé para qué es esto, lo único que sé, es que se trata
                     // de una función.
    NULL,            // "child_setup_data", este argumento es pasado a "child_setup"
    NULL,            // "child_setup_data_destroy", un "GDestroyNotify" para "child_setup_data"
    -1,              // Tiempo de espera para que aparezca lo que hemos indicado en argv, -1 = infinito.
    NULL,            // "cancellable", un "GCancellable". Nose, espero aberte ayudado
    NULL,            // "callback", un callback del tipo "VteTerminalSpawnAsyncCallback".
    NULL             // "user_data" (un gpointer [void *], puede ser cualquier cosa)
  );
}

// Esta función es llamada por app (ver las variables definidas al principio), en el signal "activate" y es
// básicamente el cuerpo principal de todo el programa.
void appActivate(GtkApplication *app, gpointer data) {
  // Cargamos la interfaz y asignamos los objetos
  builder         =                  gtk_builder_new_from_file("data/ui/instalarch-concept-2.ui");
  window          = (GtkWindow*)     gtk_builder_get_object(builder, "window");
  exitBtn         = (GtkButton*)     gtk_builder_get_object(builder, "exitBtn");
  backBtn         = (GtkButton*)     gtk_builder_get_object(builder, "backBtn");
  nextBtn         = (GtkButton*)     gtk_builder_get_object(builder, "nextBtn");
  stack           = (GtkStack*)      gtk_builder_get_object(builder, "stack");
  deskImgStack    = (GtkStack*)      gtk_builder_get_object(builder, "desktopImgStack");
  deskImgSwitcher = (GtkListBox*)    gtk_builder_get_object(builder, "desktopImgSwitcher");
  deskRow1        = (GtkListBoxRow*) gtk_builder_get_object(builder, "deskRow1");
  header          = (GtkHeaderBar*)  gtk_builder_get_object(builder, "stack");
  termView        = (GtkViewport*)   gtk_builder_get_object(builder, "termView");
  term            = (VteTerminal*)   vte_terminal_new(); // Creamos nuestra instancia de la terminal
  spawnShell(term, 0, NULL); // Hacemos que aparezca nuestra shell

  // Añadimos el widget de la terminal en la página "Progreso de la instalación"
  gtk_container_add((GtkContainer*) termView, (GtkWidget*) term);
  // Establecemos la página visible por defecto en la vista de previews de escritorios
  gtk_stack_set_visible_child_name(deskImgStack, "desktopImg1");
  // Esto es para hacer coincidir la celda seleccionada con la página activa
  gtk_list_box_select_row(deskImgSwitcher, deskRow1);

  // Conectamos signals correspondientes
  g_signal_connect(exitBtn, "clicked", (GCallback) exitBtn_clicked, NULL);
  g_signal_connect(backBtn, "clicked", (GCallback) mainPageSwitcher, NULL);
  g_signal_connect(nextBtn, "clicked", (GCallback) mainPageSwitcher, NULL);
  g_signal_connect(deskImgSwitcher, "row-selected", (GCallback) deskImgOnSwitch, NULL);
  // Para evitar que la shell se cierre, al momento en que el usuario ejecuta "exit", la función
  // spawnShell es llamada nuevamente.
  g_signal_connect(term, "child-exited", (GCallback) spawnShell, NULL);

  // Mostramos todos los widgets dentro de "window"
  gtk_widget_show_all((GtkWidget*) window);
  // Y agregamos nuestra ventana a la applicación :)
  gtk_application_add_window(app, window);
}

int main(int argc, char **argv) {
  int status; // Código de estado

  // Creamos una nueva applicación
  app = gtk_application_new("com.github.M1que4s.Instalarch", G_APPLICATION_FLAGS_NONE);
  // Conectamos la función previa con nuestra appliación
  g_signal_connect(app, "activate", (GCallback) appActivate, NULL);

  // Corremos la appliación
  status = g_application_run((GApplication*) app, argc, argv);
  // Sinceramente, no sé que hace esto xd véase la documentación de GObject para más información.
  g_object_unref(app);

  return status;
}