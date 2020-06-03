#include <gtk/gtk.h>
#include <vte/vte.h>
#define _(String) gettext(String)
#define N_(String) String

GtkApplication *app;
GtkBuilder     *builder;

GtkWindow        *window;
GtkMessageDialog *exitDialog;
GtkStack         *stack;
GtkButton        *backBtn, *nextBtn, *exitBtn;
GtkViewport      *termView;
VteTerminal      *term;
VtePty           *pty;

const gchar *stackPages[] = { "welcome", "language", "disk", "terminal" };

void switchPage(GtkButton *btn, gpointer dir) {
  const gchar *page = gtk_stack_get_visible_child_name(stack);
  const gchar *btnName = gtk_widget_get_name((GtkWidget*) btn);
  gint iter = 0;

  for (iter; iter < G_N_ELEMENTS(stackPages); iter++) {
    if (g_strcmp0(page, stackPages[iter]) == 0) {
      if (g_strcmp0(btnName, "backBtn") == 0) gtk_stack_set_visible_child_name(stack, stackPages[iter - 1]);
      if (g_strcmp0(btnName, "nextBtn") == 0) gtk_stack_set_visible_child_name(stack, stackPages[iter + 1]);
    }
  }
}

void exitBtn_clicked(GtkButton *btn, gpointer data) {
  exitDialog = (GtkMessageDialog*) gtk_builder_get_object(builder, "exitDialog");
  int res = gtk_dialog_run(GTK_DIALOG(exitDialog));

  switch (res) {
    case GTK_RESPONSE_OK:
      g_application_quit((GApplication*) app);
      break;
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_CANCEL:
      gtk_widget_hide((GtkWidget*) exitDialog);
      break;
    default:
      printf("No action for %d\n", res);
      break;
  }
}

void spawnShell(VteTerminal *term, gint status, gpointer data) {
  char **termArgv = (char *[]) { getenv("SHELL"), NULL };
  vte_terminal_spawn_async(term, VTE_PTY_DEFAULT, NULL, termArgv, NULL, G_SPAWN_DEFAULT,
                           NULL, NULL, NULL, -1, NULL, NULL, NULL);
}

void appActivate(GtkApplication *app, gpointer data) {
  builder  = (GtkBuilder*)  gtk_builder_new_from_file("data/ui/instalarch-concept-2.ui");
  window   = (GtkWindow*)   gtk_builder_get_object(builder, "window");
  exitBtn  = (GtkButton*)   gtk_builder_get_object(builder, "exitBtn");
  backBtn  = (GtkButton*)   gtk_builder_get_object(builder, "backBtn");
  nextBtn  = (GtkButton*)   gtk_builder_get_object(builder, "nextBtn");
  stack    = (GtkStack*)    gtk_builder_get_object(builder, "stack");
  termView = (GtkViewport*) gtk_builder_get_object(builder, "termView");
  term     = (VteTerminal*) vte_terminal_new();
  spawnShell(term, 0, NULL);

  gtk_container_add((GtkContainer*) termView, (GtkWidget*) term);

  g_signal_connect(exitBtn, "clicked", (GCallback) exitBtn_clicked, NULL);
  g_signal_connect(backBtn, "clicked", (GCallback) switchPage, NULL);
  g_signal_connect(nextBtn, "clicked", (GCallback) switchPage, NULL);
  g_signal_connect(term, "child-exited", (GCallback) spawnShell, NULL);

  gtk_widget_show_all((GtkWidget*) window);
  gtk_application_add_window(app, window);
}

int main(int argc, char **argv) {
  int status;

  app = gtk_application_new("com.github.M1que4s.Instalarch", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", (GCallback) appActivate, NULL);

  status = g_application_run((GApplication*) app, argc, argv);
  g_object_unref(app);

  return status;
}