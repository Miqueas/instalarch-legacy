#include <gtk/gtk.h>
#define _(String) gettext(String)
#define N_(String) String

GtkApplication *app;
GtkBuilder     *builder;

GtkWindow        *window;
GtkButton        *exitBtn;
GtkMessageDialog *exitDialog;

void exitBtn_clicked(GtkButton *btn, gpointer data) {
  exitDialog = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder, "exitDialog"));
  int res = gtk_dialog_run(GTK_DIALOG(exitDialog));

  switch (res) {
    case GTK_RESPONSE_OK:
      g_application_quit(G_APPLICATION(app));
      break;
    case GTK_RESPONSE_CANCEL:
      gtk_widget_hide(GTK_WIDGET(exitDialog));
      break;
    default:
      printf("No action for %d\n", res);
      break;
  }
}

void appActivate(GtkApplication *app, gpointer data) {
  builder = GTK_BUILDER(gtk_builder_new_from_file("data/ui/instalarch-concept-2.ui"));
  window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));
  exitBtn = GTK_BUTTON(gtk_builder_get_object(builder, "exitBtn"));
  g_signal_connect(exitBtn, "clicked", G_CALLBACK(exitBtn_clicked), NULL);

  gtk_widget_show_all(GTK_WIDGET(window));
  gtk_application_add_window(app, window);
}

int main(int argc, char **argv) {
  int status;

  app = gtk_application_new("com.github.M1que4s.Instalarch", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(appActivate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}