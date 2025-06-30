
#include <KFilePlacesView>

namespace KDFM
{

class FilePlacesView : public KFilePlacesView
{
    Q_OBJECT
public:
    FilePlacesView(QWidget *parent = 0);
    ~FilePlacesView();
//    QRect visualRect(const QModelIndex &index) const;
//    void doItemsLayout();
    QModelIndex indexAt(const QPoint &p) const;

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
//    void paintEvent(QPaintEvent *e);
//    void resizeEvent(QResizeEvent *e);
//    void contextMenuEvent(QContextMenuEvent *e);
//    bool eventFilter(QObject *o, QEvent *e);
//    int verticalOffset() const;

signals:
    void newTabRequest(const QUrl &url);

private:
    class Private;
    Private * const d;
    friend class Private;
};

}
