#include "StackPlaneView.h"
#include "StackSession.h"
#include "StackPlaneController.h"

#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkInteractorStyle.h>
#include <vtkPointData.h>
#include "QVTKWidget.h"
#include <QtGui/QWidget>

using namespace NeuroProof;
using std::tr1::unordered_set;

StackPlaneView::StackPlaneView(StackSession* stack_session_, 
        StackPlaneController* controller_, QWidget* widget_parent_) : 
        stack_session(stack_session_), controller(controller_),
        widget_parent(widget_parent_), renderWindowInteractor(0)
{
    stack_session->attach_observer(this);
} 

StackPlaneView::~StackPlaneView()
{
    stack_session->detach_observer(this);
    if (qt_widget) {
        delete qt_widget;
    }
}
    
void StackPlaneView::pan(int xdiff, int ydiff)
{
    double viewFocus[4], viewPoint[3];

    vtkRenderer * renderer = viewer->GetRenderer();  
    vtkCamera *camera = renderer->GetActiveCamera();

    camera->GetFocalPoint(viewFocus);
    camera->GetPosition(viewPoint);
    
    camera->SetFocalPoint(xdiff + viewFocus[0], ydiff + viewFocus[1],
            viewFocus[2]);
    camera->SetPosition(xdiff + viewPoint[0], ydiff + viewPoint[1],
            viewPoint[2]);

    if (renderWindowInteractor->GetLightFollowCamera()) {
        renderer->UpdateLightsGeometryToFollowCamera();
    }
    renderWindowInteractor->Render();
}

// call update and create controller -- rag, gray, and labels must exist
void StackPlaneView::initialize()
{
    Stack* stack = stack_session->get_stack();

    VolumeLabelPtr labelvol = stack->get_labelvol();  
    VolumeGrayPtr grayvol = stack->get_grayvol();
    if (!grayvol) {
        throw ErrMsg("Cannot Initialize: no gray volume loaded");
    }
    
    // load gray volume into vtk array
    grayarray = vtkSmartPointer<vtkUnsignedCharArray>::New();
    grayarray->SetArray(grayvol->begin(), grayvol->shape(0) * 
            grayvol->shape(1) * grayvol->shape(2), 1);
    
    // load array into gray vtk image volume
    grayvtk = vtkSmartPointer<vtkImageData>::New();
    grayvtk->GetPointData()->SetScalars(grayarray); 

    // set grayscale properties
    grayvtk->SetDimensions(grayvol->shape(0), grayvol->shape(1),
            grayvol->shape(2));
    grayvtk->SetScalarType(VTK_UNSIGNED_CHAR);
    grayvtk->SetSpacing(1, 1, 1); // hack for now; TODO: set res from stack?
    grayvtk->SetOrigin(0.0, 0.0, 0.0);

    graylookup = vtkSmartPointer<vtkLookupTable>::New();
    graylookup->SetNumberOfTableValues(256);
    graylookup->SetRange(0.0, 256);
    graylookup->SetHueRange(0.0,1.0);
    graylookup->SetValueRange(0.0,1.0);
    graylookup->Build();
    for (int i = 0; i < 256; ++i) {
        graylookup->SetTableValue(i, i/255.0, i/255.0, i/255.0, 1);
    }
    gray_mapped = vtkSmartPointer<vtkImageMapToColors>::New();
    gray_mapped->SetInput(grayvtk);
    gray_mapped->SetLookupTable(graylookup);
    gray_mapped->Update();

    // rebase label volume and enable show all labels in a different color
    unsigned int * labels_rebase = new unsigned int [labelvol->shape(0) * labelvol->shape(1) * labelvol->shape(2)];
    unsigned int * iter = labels_rebase;
    Label_t max_label = 0;
    volume_forXYZ(*labelvol, x, y, z) {
        Label_t label = (*labelvol)(x,y,z);
        if (label > max_label) {
            max_label = label;
        }
        *iter++ = label;
    }
    labelarray = vtkSmartPointer<vtkUnsignedIntArray>::New();
    labelarray->SetArray(labels_rebase, labelvol->shape(0) * labelvol->shape(1) *
            labelvol->shape(2), 0);

    // set label options
    labelvtk = vtkSmartPointer<vtkImageData>::New();
    labelvtk->GetPointData()->SetScalars(labelarray);
    labelvtk->SetDimensions(labelvol->shape(0), labelvol->shape(1),
            labelvol->shape(2));
    labelvtk->SetScalarType(VTK_UNSIGNED_INT);
    labelvtk->SetSpacing(1, 1, 1);
    labelvtk->SetOrigin(0.0, 0.0, 0.0);

    // set lookup table
    label_lookup = vtkSmartPointer<vtkLookupTable>::New();
    label_lookup->SetNumberOfTableValues(max_label+1);
 //           stack_session->get_stack()->get_num_labels());
//    std::cout << stack_session->get_stack()->get_num_labels()  << std::endl;
//    std::cout << max_label << std::endl;
    label_lookup->SetRange( 0.0, max_label); 
    label_lookup->SetHueRange( 0.0, 1.0 );
    label_lookup->SetValueRange( 0.0, 1.0 );
    label_lookup->Build();

    // compute different colors for each RAG node
    // using greedy graph coloring algorithm
    stack_session->compute_label_colors();

    RagPtr rag = stack->get_rag();
    for (Rag_t::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        int color_id = (*iter)->get_property<int>("color");
        unsigned char r, g, b;
        stack_session->get_rgb(color_id, r, g, b);
        label_lookup->SetTableValue((*iter)->get_node_id(), r/255.0,
               g/255.0, b/255.0, 1);
    }
    
    // map colors for each label
    labelvtk_mapped = vtkSmartPointer<vtkImageMapToColors>::New();
    labelvtk_mapped->SetInput(labelvtk);
    labelvtk_mapped->SetLookupTable(label_lookup);
    labelvtk_mapped->Update();


    // blend both images
    vtkblend = vtkSmartPointer<vtkImageBlend>::New();
    vtkblend->AddInputConnection(gray_mapped->GetOutputPort());
    vtkblend->AddInputConnection(labelvtk_mapped->GetOutputPort());
    vtkblend->SetOpacity(0, 1);
    vtkblend->SetOpacity(1, 0.3);
    vtkblend->Update();
    
    // flip along y
    vtkblend_flipped = vtkSmartPointer<vtkImageFlip>::New();
    vtkblend_flipped->SetInputConnection(vtkblend->GetOutputPort());
    vtkblend_flipped->SetFilteredAxis(1);
    vtkblend_flipped->Update();


    // create 2D view
    viewer = vtkSmartPointer<vtkImageViewer2>::New();
    viewer->SetColorLevel(127.5);
    viewer->SetColorWindow(255);
    viewer->SetInputConnection(vtkblend_flipped->GetOutputPort());

    //qt widgets
   
    if (widget_parent) {
        qt_widget = new QVTKWidget(widget_parent);
        //planeView->setGeometry(QRect(0, 0, 671, 571));
    } else {
        qt_widget = new QVTKWidget;
    }
    //qt_widget->setObjectName(QString::fromUtf8("planeView"));
    qt_widget->showMaximized();
    qt_widget->SetRenderWindow(viewer->GetRenderWindow());
    renderWindowInteractor = qt_widget->GetInteractor();

    viewer->SetupInteractor(renderWindowInteractor);
    //viewer->SetSlice(0);
    //viewer->GetRenderer()->ResetCamera();
    viewer->Render();
}

void StackPlaneView::start()
{
    qt_widget->show();
}

void StackPlaneView::update()
{
    bool show_all;
    unsigned int plane_id;
    Label_t select_id = 0;
    Label_t select_id_old = 0;
    Label_t ignore_label = 0;
    double rgba[4];
    unordered_set<Label_t> active_labels;

    bool show_all_change = stack_session->get_show_all(show_all);
    double opacity_val = 1.0;
    if (!show_all) {
        opacity_val = 0.0;
    }

    if (stack_session->get_active_labels(active_labels)) {
        for (int i = 0; i < label_lookup->GetNumberOfTableValues(); ++i) {
            label_lookup->GetTableValue(i, rgba);
            rgba[3] = 0;
            label_lookup->SetTableValue(i, rgba);
        }    
        for (unordered_set<Label_t>::iterator iter = active_labels.begin();
                iter != active_labels.end(); ++iter) {
            label_lookup->GetTableValue(*iter, rgba);
            rgba[3] = opacity_val;
            label_lookup->SetTableValue(*iter, rgba);
        }
    }

    if (stack_session->get_select_label(select_id, select_id_old)) {
        if (select_id_old && (active_labels.empty() ||
                (active_labels.find(select_id_old) != active_labels.end())) ) {
            label_lookup->GetTableValue(select_id_old, rgba);
            rgba[3] = 1.0;
            label_lookup->SetTableValue(select_id_old, rgba);
            label_lookup->Modified();
        } 
    }
   
    if (select_id) {
        // do not toggle this label on when doing global toggle
        ignore_label = select_id;
    }

    if (show_all_change) {
        double opacity_val = 1.0;
        if (!show_all) {
            opacity_val = 0.0;
        }
        double rgba[4];

        if (!active_labels.empty()) {
            for (unordered_set<Label_t>::iterator iter = active_labels.begin();
                    iter != active_labels.end(); ++iter) {
                label_lookup->GetTableValue(*iter, rgba);
                rgba[3] = opacity_val;
                label_lookup->SetTableValue(*iter, rgba);
            }
        } else { 
            for (int i = 0; i < label_lookup->GetNumberOfTableValues(); ++i) {
                label_lookup->GetTableValue(i, rgba);
                rgba[3] = opacity_val;
                label_lookup->SetTableValue(i, rgba);
            }
        }
        label_lookup->Modified();
    }

    if (ignore_label) {
        label_lookup->GetTableValue(ignore_label, rgba);
        rgba[3] = 0.0;
        label_lookup->SetTableValue(ignore_label, rgba);
        label_lookup->Modified();
    }

    if (stack_session->get_plane(plane_id)) {
        viewer->SetSlice(plane_id);
    }

    viewer->Render();
}
