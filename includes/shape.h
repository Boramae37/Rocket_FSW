#ifndef shape


class Rectangle {
 public:
  Rectangle(int width, int height);

  int GetSize() const;

 private:
  int width_, height_;
};



#endif