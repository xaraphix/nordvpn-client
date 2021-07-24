/* Open GL Area
 *
 * Gtk::GLArea is a widget that allows custom drawing using OpenGL calls.
 *
 */

#include <iostream>
#include <string>
#include <vector>

#include <gtkmm.h>
#include <giomm/resource.h>
#include <epoxy/gl.h>

using std::cerr;
using std::endl;
using std::string;

enum {
  X_AXIS,
  Y_AXIS,
  Z_AXIS,

  N_AXIS
};

static const GLfloat vertex_data[] = {
  0.f,   0.5f,   0.f, 1.f,
  0.5f, -0.366f, 0.f, 1.f,
 -0.5f, -0.366f, 0.f, 1.f,
};

class Example_GLArea : public Gtk::Window
{
public:
  Example_GLArea();
  ~Example_GLArea() override;

protected:
  Gtk::Box m_VBox {Gtk::Orientation::VERTICAL, false};
  Gtk::GLArea m_GLArea;
  Gtk::Box m_Controls {Gtk::Orientation::VERTICAL, false};
  Gtk::Button m_Button {"Quit"};

  GLuint m_Vaooooo {0};
  GLuint m_Buffer {0};
  GLuint m_Program {0};
  GLuint m_Mvp {0};

  std::vector<float> m_RotationAngles;

  void on_axis_value_change(int axis, const Glib::RefPtr<Gtk::Adjustment>& adj);

  void realize();
  void unrealize();
  bool render(const Glib::RefPtr<Gdk::GLContext>& context);

  Gtk::Box* create_axis_slider_box(int axis);
  void init_buffers();
  void init_shaders(const std::string& vertex_path, const std::string& fragment_path);
  void draw_triangle();
};

Gtk::Window* do_glarea()
{
  return new Example_GLArea();
}

Example_GLArea::Example_GLArea() : m_RotationAngles(N_AXIS, 0.0f)
{
  set_decorated(false);
  set_default_size(400, 600);

  m_VBox.set_margin(0);
  m_VBox.set_spacing(0);
  set_child(m_VBox);

  m_GLArea.set_expand(true);
  m_GLArea.set_size_request(100, 200);
  m_GLArea.set_auto_render(true);
  m_VBox.append(m_GLArea);

  // Connect gl area signals
  m_GLArea.signal_realize().connect(sigc::mem_fun(*this, &Example_GLArea::realize));
  // Important that the unrealize signal calls our handler to clean up
  // GL resources _before_ the default unrealize handler is called (the "false")
  m_GLArea.signal_unrealize().connect(sigc::mem_fun(*this, &Example_GLArea::unrealize), false);
  m_GLArea.signal_render().connect(sigc::mem_fun(*this, &Example_GLArea::render), false);

  // Connect clicked to close of window
  m_Button.signal_clicked().connect(sigc::mem_fun(*this, &Gtk::Window::close));
}

Example_GLArea::~Example_GLArea()
{
}

void Example_GLArea::on_axis_value_change(int axis, const Glib::RefPtr<Gtk::Adjustment>& adj)
{
  m_RotationAngles[axis] = adj->get_value();
  m_GLArea.queue_draw();
}

void Example_GLArea::realize()
{
  m_GLArea.make_current();
  try
  {
    m_GLArea.throw_if_error();
    init_buffers();

    const bool use_es = m_GLArea.get_context()->get_use_es();
    const std::string vertex_path = use_es ? "/glarea/v.glsl" : "/glarea/v.glsl";
    const std::string fragment_path = use_es ? "/glarea/f.glsl" : "/glarea/f.glsl";
    init_shaders(vertex_path, fragment_path);
  }
  catch(const Gdk::GLError& gle)
  {
    cerr << "An error occured making the context current during realize:" << endl;
    cerr << gle.domain() << "-" << gle.code() << "-" << gle.what() << endl;
  }
}

void Example_GLArea::unrealize()
{
  m_GLArea.make_current();
  try
  {
    m_GLArea.throw_if_error();

    // Delete buffers and program
    glDeleteBuffers(1, &m_Buffer);
    glDeleteProgram(m_Program);
  }
  catch(const Gdk::GLError& gle)
  {
    cerr << "An error occured making the context current during unrealize" << endl;
    cerr << gle.domain() << "-" << gle.code() << "-" << gle.what() << endl;
  }
}

bool Example_GLArea::render(const Glib::RefPtr<Gdk::GLContext>& /* context */)
{
  try
  {
    m_GLArea.throw_if_error();
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    draw_triangle();

    glFlush();

    return true;
  }
  catch(const Gdk::GLError& gle)
  {
    cerr << "An error occurred in the render callback of the GLArea" << endl;
    cerr << gle.domain() << "-" << gle.code() << "-" << gle.what() << endl;
    return false;
  }
}

void Example_GLArea::init_buffers()
{
  glGenVertexArrays(1, &m_Vaooooo);
  glBindVertexArray(m_Vaooooo);

  glGenBuffers(1, &m_Buffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static GLuint create_shader(int type, const char *src)
{
  auto shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  int status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if(status == GL_FALSE)
  {
    int log_len;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

    string log_space(log_len+1, ' ');
    glGetShaderInfoLog(shader, log_len, nullptr, (GLchar*)log_space.c_str());

    cerr << "Compile failure in " <<
      (type == GL_VERTEX_SHADER ? "vertex" : "fragment") <<
      " shader: " << log_space << endl;

    glDeleteShader(shader);

    return 0;
  }

  return shader;
}

void Example_GLArea::init_shaders(const std::string& vertex_path, const std::string& fragment_path)
{
  auto vshader_bytes = Gio::Resource::lookup_data_global(vertex_path);
  if(!vshader_bytes)
  {
    cerr << "Failed fetching vertex shader resource" << endl;
    m_Program = 0;
    return;
  }
  gsize vshader_size {vshader_bytes->get_size()};
  auto vertex = create_shader(GL_VERTEX_SHADER,
                         (const char*)vshader_bytes->get_data(vshader_size));

  if(vertex == 0)
  {
    m_Program = 0;
    return;
  }

  auto fshader_bytes = Gio::Resource::lookup_data_global(fragment_path);
  if(!fshader_bytes)
  {
    cerr << "Failed fetching fragment shader resource" << endl;
    glDeleteShader(vertex);
    m_Program = 0;
    return;
  }
  gsize fshader_size {fshader_bytes->get_size()};
  auto fragment = create_shader(GL_FRAGMENT_SHADER,
                           (const char*)fshader_bytes->get_data(fshader_size));

  if(fragment == 0)
  {
    glDeleteShader(vertex);
    m_Program = 0;
    return;
  }

  m_Program = glCreateProgram();
  glAttachShader(m_Program, vertex);
  glAttachShader(m_Program, fragment);

  glLinkProgram(m_Program);

  int status;
  glGetProgramiv(m_Program, GL_LINK_STATUS, &status);
  if(status == GL_FALSE)
  {
    int log_len;
    glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &log_len);

    string log_space(log_len+1, ' ');
    glGetProgramInfoLog(m_Program, log_len, nullptr, (GLchar*)log_space.c_str());

    cerr << "Linking failure: " << log_space << endl;

    glDeleteProgram(m_Program);
    m_Program = 0;
  }
  else
  {
    /* Get the location of the "mvp" uniform */
    m_Mvp = glGetUniformLocation(m_Program, "mvp");

    glDetachShader(m_Program, vertex);
    glDetachShader(m_Program, fragment);
  }
  glDeleteShader(vertex);
  glDeleteShader(fragment);
}

static void compute_mvp(float *res,
                        float phi,
                        float theta,
                        float psi)
{
  float x       {phi * ((float)G_PI / 180.f)};
  float y       {theta * ((float)G_PI / 180.f)};
  float z       {psi * ((float)G_PI / 180.f)};
  float c1      {cosf (x)};
  float s1      {sinf (x)};
  float c2      {cosf (y)};
  float s2      {sinf (y)};
  float c3      {cosf (z)};
  float s3      {sinf (z)};
  float c3c2    {c3 * c2};
  float s3c1    {s3 * c1};
  float c3s2s1  {c3 * s2 * s1};
  float s3s1    {s3 * s1};
  float c3s2c1  {c3 * s2 * c1};
  float s3c2    {s3 * c2};
  float c3c1    {c3 * c1};
  float s3s2s1  {s3 * s2 * s1};
  float c3s1    {c3 * s1};
  float s3s2c1  {s3 * s2 * c1};
  float c2s1    {c2 * s1};
  float c2c1    {c2 * c1};

  /* apply all three rotations using the three matrices:
   *
   * ⎡  c3 s3 0 ⎤ ⎡ c2  0 -s2 ⎤ ⎡ 1   0  0 ⎤
   * ⎢ -s3 c3 0 ⎥ ⎢  0  1   0 ⎥ ⎢ 0  c1 s1 ⎥
   * ⎣   0  0 1 ⎦ ⎣ s2  0  c2 ⎦ ⎣ 0 -s1 c1 ⎦
   */
  res[0] = c3c2;  res[4] = s3c1 + c3s2s1;  res[8] = s3s1 - c3s2c1; res[12] = 0.f;
  res[1] = -s3c2; res[5] = c3c1 - s3s2s1;  res[9] = c3s1 + s3s2c1; res[13] = 0.f;
  res[2] = s2;    res[6] = -c2s1;         res[10] = c2c1;          res[14] = 0.f;
  res[3] = 0.f;   res[7] = 0.f;           res[11] = 0.f;           res[15] = 1.f;
}

void Example_GLArea::draw_triangle()
{
  float mvp[16];

  compute_mvp(mvp,
              m_RotationAngles[X_AXIS],
              m_RotationAngles[Y_AXIS],
              m_RotationAngles[Z_AXIS]);

  glUseProgram(m_Program);

  glUniformMatrix4fv(m_Mvp, 1, GL_FALSE, &mvp[0]);

  glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

  glDrawArrays(GL_TRIANGLES, 0, 3);

  glDisableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);
}


int main(int argc, char* argv[])
{
 auto app = Gtk::Application::create("org.gtkmm.examples.base"); 
 return app->make_window_and_run<Example_GLArea>(argc, argv); 
}

