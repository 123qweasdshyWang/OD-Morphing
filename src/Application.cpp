#include "Application.h"


Application::Application()
{
}


Application::~Application()
{
    this->TerminateOpenGL();
}


void Application::TerminateOpenGL()
{
    // 6. Terminate EGL when finished
    eglTerminate(this->eglDpy);
}


void Application::InitOpenGL()
{
    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };

    const int pbufferWidth = 200;
    const int pbufferHeight = 200;

    const EGLint pbufferAttribs[] = {
        EGL_WIDTH, pbufferWidth,
        EGL_HEIGHT, pbufferHeight,
        EGL_NONE,
    };

    // 1. Initialize EGL
    const int MAX_DEVICES = 10;
    EGLDeviceEXT eglDevs[MAX_DEVICES];
    EGLint numDevices;

    PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT =
        (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");

    eglQueryDevicesEXT(MAX_DEVICES, eglDevs, &numDevices);

    std::cout << "Detected " << numDevices << " devices" << std::endl;

    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");

    this->eglDpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, eglDevs[0], 0);

    // If you want to use on-screen rendering,
    // just delete the code up there.
    // And use following line:
    // this->eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint major, minor;

    eglInitialize(this->eglDpy, &major, &minor);

    // 2. Select an appropriate configuration
    EGLint numConfigs;
    EGLConfig eglCfg;

    eglChooseConfig(this->eglDpy, configAttribs, &eglCfg, 1, &numConfigs);

    // 3. Create a surface
    EGLSurface eglSurf = eglCreatePbufferSurface(this->eglDpy, eglCfg, pbufferAttribs);

    // 4. Bind the API
    eglBindAPI(EGL_OPENGL_API);

    // 5. Create a context and make it current
    EGLContext eglCtx = eglCreateContext(this->eglDpy, eglCfg, EGL_NO_CONTEXT, nullptr);

    if (eglCtx == nullptr) {
        throw RenderException("eglCreateContext failed");
    }

    eglMakeCurrent(this->eglDpy, eglSurf, eglSurf, eglCtx);
}


void Application::Input(const char * inputFile)
{

    using std::vector;
    using std::string;
    using std::fstream;
    using std::stringstream;
    using std::bitset;

    fstream input;
    input.open(inputFile, std::fstream::in);

    if (!input) {
        throw RenderException("Could not open input file");
    }

    input >> this->dataSet;
/*
    const int frameSize = 100;

    frames.resize(frameSize);
    bitset<3000> state[frameSize], zero;

    size_t lineNum = 0;
    for (string stringLine; getline(input, stringLine); ) {
        stringstream lineBuffer(stringLine);
        string token;

        // split by ','
        float number[4];
        for (int i = 0; i < 4; i++) {
            getline(lineBuffer, token, ',');
            stringstream buffer(token);
            buffer >> number[i];
        }

        Line line;
        //vector<Record> records;
        line.AddPoint(Point(number[1], number[0], 1.0f));
        line.AddPoint(Point(number[3], number[2], 1.0f));
        this->dataSet.lines.push_back(line);

        for (int i = 0; i < frameSize; i ++) {
            int in;
            getline(lineBuffer, token, ',');
            stringstream buffer(token);
            buffer >> in;
            state[i].set(lineNum, (bool)in);
        }
        lineNum ++;
    }

    for (int i = 0; i < frameSize; i ++) {
        const auto & prev = i == 0 ? zero : state[i - 1];
        for (int j = 0; j < lineNum; j ++) {
            if (prev[j] != state[i][j]) {
                if (state[i][j]) {
                    this->frames[i].add.push_back(j);
                } else {
                    this->frames[i].remove.push_back(j);
                }
            }
        }
    }
*/

    //
    //

    input.close();
}


void Application::Output(const char * outputFile)
{
    std::fstream output;
    output.open(outputFile, std::fstream::out);

    if (!output) {
        throw RenderException("Could not open output file");
    }

    output << this->dataSet;
    output.close();
}


void Application::Config()
{
    int count, textureWidth;
    float kernelSize;
    double attractionFactor, smoothingFactor;
    bool doResampling;
    int resampleStep;
  
    count = 10;
    textureWidth = 200;
    kernelSize = 20;
    attractionFactor = 0.01;
    smoothingFactor = 0.1;doResampling = true;
    resampleStep = 1;
  
    this->iterationsForNonRealTime.push_back(Iteration(count, textureWidth, kernelSize, attractionFactor, smoothingFactor, doResampling, resampleStep));
}


int Application::Run(int layerNum, double maxLng, double minLng, double maxLat, double minLat)
{
    //this->inputPath = inputFile;

    clock_t start, end;
    start = clock();

    this->LngRecord = maxLng; this->LatRecord = maxLat;

    this->InitOpenGL();

    this->Config();

    //this->Input(inputFile);
    int tripsNum = this->Input(layerNum, maxLng, minLng, maxLat, minLat);

    if (tripsNum == 0) {
        this->TerminateOpenGL();
        return 1;
    }

    this->NonRealTimeBundleWithWaypoint();
    //this->NonRealTimeBundle();

    //this->Output("xx");

    this->TerminateOpenGL();

    end = clock();
    std::cout<< "whole total time: " << (double)(end-start)/CLOCKS_PER_SEC << std::endl;

    return 0;
}



void Application::NonRealTimeBundleWithWaypoint()
{
    using std::fstream;
    using std::string;
    using std::stringstream;

    this->InitBundle();

    this->dataSet.CreateGridGraph();

    for (int fileNum = 1; fileNum <= 26; fileNum ++) {

        for (size_t i = 0; i < this->dataSet.lines.size(); i ++) {
            Line & line = this->dataSet.lines[i];
            if (line.GetPointSize() > 0 && line.id > 1000000) {
                std::cout << i << std::endl;
                throw "line id error in Application::NonRealTimeBundleWithWaypoint";
            }
        }
        this->dataSet.UpdateWaypoints(this->refPoints);

        // get i-th line, j-th segment
        // RoadSegment segment = this->dataSet.roads[i][j]
        // segment.oId
        // segment.dId
        // segment.density

        // bundle
        //#pragma omp parallel for
        for (size_t i = 0; i < this->dataSet.lines.size(); i ++) {
            Line & line = this->dataSet.lines[i];

            if (line.GetWaypointSize() == 0) {
                line = Line();
            }
            // std::cout << "Line " << i << std::endl;
            line.UpdatePoints();
        }


        // Iteration(count, textureWidth, kernelSize, attractionFactor, smoothingFactor, doResampling))
        //this->BundleWithWaypoint(Iteration(20, 200, 20, 0.001, 0.1, true));
        this->BundleWithWaypoint(this->iterationsForNonRealTime[0], fileNum);


        //#pragma omp parallel for
        for (size_t i = 0; i < this->dataSet.lines.size(); i ++) {
            Line & line = this->dataSet.lines[i];
            line.ClearWaypoints();
        }

    }
}


void Application::NonRealTimeBundle()
{
    this->InitBundle();
    for (const Iteration & iteration : this->iterationsForNonRealTime) {
        this->Bundle(iteration);
    }
}

/*
void Application::RealTimeBundle()
{
    this->InitBundle();
    DataSet * originalDataSet = new DataSet(this->dataSet);

    for (size_t i = 0; i < this->frames.size(); i ++) {
        std::cout << i << std::endl;
        const auto & frame = this->frames[i];

        for (const size_t index: frame.add) {
            this->dataSet.data[index] = originalDataSet->data[index];
        }
        for (const size_t index: frame.remove) {
            this->dataSet.data[index].clear();
        }
        // Iteration(count, textureWidth, kernelSize, attractionFactor, smoothingFactor, doResampling))
        this->Bundle(Iteration(10, 100, 20, 0.1, 0.1, true));

        // for debug
        std::stringstream buffer;
        buffer << "realtime_" << i;
        this->Output(buffer.str().c_str());
        //
    }
    delete originalDataSet;
}
*/

void Application::InitBundle()
{
    // initial shader
    this->shader.LoadFragmentShader();
    this->shader.LoadVertexShader();
    this->shader.CreateProgram();
}


/*
 * Kernel Density Estimation-based Edge Bundling
 */
void Application::Bundle(const Iteration & iteration)
{
    const uint32_t textureWidth = iteration.textureWidth;

    Framebuffer framebuffer(iteration.textureWidth);
    framebuffer.Init();

    this->gradient.Resize(textureWidth * textureWidth);
    this->gradient.SetWidth(textureWidth);
    this->gradient.SetAttractionFactor(iteration.attractionFactor);

    for (int i = 0; i < iteration.count; i++) {
        Image image(dataSet, &this->shader);
        image.Init();

        std::vector<float> accD = framebuffer.ComputeSplatting(iteration.kernelSize, image);
        this->gradient.SetAccMap(&accD);
        this->gradient.ApplyGradient(this->dataSet);

        this->dataSet.SmoothTrails(iteration.smoothingFactor);
        if (iteration.doResampling) {
            this->dataSet.AddRemovePoints(this->pointRemoveDistance, this->pointSplitDistant);
        }
    }
}

/*
void loadOutputToDB(int iterNum, double LngRecord, double LatRecord) 
{
        std::this_thread::sleep_for(std::chrono::milliseconds((iterNum+1)%21*100));

        // call python script
        std::stringstream ss1, ss2; std::string lng, lat;
        ss1 << std::fixed << std::setprecision(3) << LngRecord; ss1 >> lng;
        ss2 << std::fixed << std::setprecision(3) << LatRecord; ss2 >> lat;
        std::string command = "/***/***/***/***//python linesformatTojson.py "+lng+"_"+lat+"_"+std::to_string(iterNum)+"";

        std::cout<< command << std::endl;
        int res = system(command.c_str());
}
*/

// just for test
// extern int num;

void Application::BundleWithWaypoint(const Iteration & iteration, int layerNum)
{
    int c = 10 * (layerNum - 1);

    const uint32_t textureWidth = iteration.textureWidth;

    Framebuffer framebuffer(iteration.textureWidth);
    framebuffer.Init();

    this->gradient.Resize(textureWidth * textureWidth);
    this->gradient.SetWidth(textureWidth);
    this->gradient.SetAttractionFactor(iteration.attractionFactor);
    this->gradient.InitSteps(iteration.count);

    for (int i = 0; i < iteration.count; i++) {
        
        Image image(dataSet, &this->shader);
        image.Init();
        std::vector<float> accD = framebuffer.ComputeSplatting(iteration.kernelSize, image);
        this->gradient.SetAccMap(&accD);
        this->gradient.ApplyGradientWithWaypoint(this->dataSet, i);
        
        if (i % iteration.resampleStep == 0 || i == iteration.count - 1) {
          this->dataSet.SmoothTrailsWithWaypoint(iteration.smoothingFactor);
          if (iteration.doResampling) {
              this->dataSet.AddRemovePointsWithWaypoint(this->pointRemoveDistance, this->pointSplitDistant);
          }
        }

/*
        if (i == iteration.count - 1) {
            this->dataSet.RemovePointsInSegment();
        }
*/

        char s[100];
        sprintf(s, "/***/***/***/***/%.3f_%.3f_%d.csv", this->LngRecord, this->LatRecord, c);
        this->Output(s);
        std::cout << "iter num: " << c << std::endl;

        //std::stringstream ss1, ss2; std::string lng, lat;
        //ss1 << std::fixed << std::setprecision(3) << this->LngRecord; ss1 >> lng;
        //ss2 << std::fixed << std::setprecision(3) << this->LatRecord; ss2 >> lat;
        //std::string command = "/***/***/***/***/python linesformatTojson.py "+lng+"_"+lat+"_"+std::to_string(c)+"";
        //int res = system(command.c_str());
        //std::cout << "iter num: " << c << " python script status: " << res << std::endl;

        //std::thread t(loadOutputToDB, c, this->LngRecord, this->LatRecord);
        //t.detach();

        c ++;
    }
}





