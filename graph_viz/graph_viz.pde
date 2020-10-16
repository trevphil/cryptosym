import java.util.Map;
import java.util.ArrayList;
import java.util.TreeSet;

class Factor {
  String ftype = "unset";
  int out = -1;
  int[] inputs = {};
  int depth = -1;
  float x = -1;
  float y = -1;

  Factor(String line) {
    String[] parts = line.split(";");
    this.ftype = parts[0];
    this.out = int(parts[1]);
    if (this.ftype.equals("PRIOR")) {
      this.inputs = new int[]{};
    } else if (this.ftype.equals("INV") || this.ftype.equals("SAME")) {
      this.inputs = new int[]{int(parts[2])};
    } else if (this.ftype.equals("AND")) {
      this.inputs = new int[]{int(parts[2]), int(parts[3])};
    }
  }

  void setX(float x) {
    this.x = x;
  }

  void setY(float y) {
    this.y = y;
  }

  int setDepth(HashMap<Integer, Factor> factors) {
    if (this.depth != -1) return this.depth;
    if (this.inputs.length == 0) {
      this.depth = 0;
      return 0;
    }

    int d1 = 1 + factors.get(inputs[0]).setDepth(factors);
    if (inputs.length > 1) {
      int d2 = 1 + factors.get(inputs[1]).setDepth(factors);
      this.depth = max(d1, d2);
      return this.depth;
    }
    this.depth = d1;
    return d1;
  }

  void render(HashMap<Integer, Factor> others) {
    float ellipseRadius = 0.5;
    noStroke();
    fill(this.depth == 0 ? color(0, 255, 0) : color(0));
    ellipse(x, y, 2 * ellipseRadius, 2 * ellipseRadius);
    for (int inp : this.inputs) {
      Factor other = others.get(inp);
      if (this.ftype.equals("AND")) {
        stroke(color(255, 0, 0));
      } else {
        stroke(color(0));
      }
      strokeWeight(0.25);
      line(this.x, this.y, other.x, other.y);
    }
  }
}

HashMap<Integer, Factor> loadFactors() {
  String[] lines = loadStrings("factors.txt");
  HashMap<Integer, Factor> factors = new HashMap<Integer, Factor>();
  for (int i = 0; i < lines.length; ++i) {
    Factor f = new Factor(lines[i]);
    factors.put(f.out, f);
  }
  return factors;
}

float depthToY(int depthIndex, int numDepthValues) {
  float availableHeight = height * 0.95;
  float topOffset = (height - availableHeight) / 2;
  float stepSize = availableHeight / numDepthValues;
  return topOffset + (stepSize * depthIndex);
}

float factorToX(Factor f, ArrayList<Factor> factorsAtSameDepth) {
  float availableWidth = width * 0.95;
  float leftOffset = (width - availableWidth) / 2;
  float stepSize = availableWidth / factorsAtSameDepth.size();
  return leftOffset + (stepSize * factorsAtSameDepth.indexOf(f));
}

void setup() {
  size(1200, 800);
  background(255);
  ellipseMode(CENTER);
  smooth();

  println("Loading factors...");
  HashMap<Integer, Factor> factors = loadFactors();
  println("Did load all " + factors.size() + " factors.");

  println("Computing depths...");
  for (Factor f : factors.values()) f.setDepth(factors);
  println("Finished computing depths.");

  println("Binning factors by depth...");
  HashMap<Integer, ArrayList<Factor>> factorsAtDepth = new HashMap<Integer, ArrayList<Factor>>();
  TreeSet<Integer> depths = new TreeSet<Integer>();
  for (Factor f : factors.values()) {
    depths.add(f.depth);
    if (!factorsAtDepth.containsKey(f.depth)) {
      ArrayList<Factor> initList = new ArrayList<Factor>();
      initList.add(f);
      factorsAtDepth.put(f.depth, initList);
    } else {
      ArrayList<Factor> previous = factorsAtDepth.get(f.depth);
      previous.add(f);
      factorsAtDepth.put(f.depth, previous);
    }
  }
  println("Did bin factors by depth.");

  int numDepthValues = depths.size();
  for (Map.Entry<Integer, ArrayList<Factor>> entry : factorsAtDepth.entrySet()) {
    int depth = entry.getKey();
    int depthIndex = depths.headSet(depth).size();
    ArrayList<Factor> factorList = entry.getValue();
    float y = depthToY(depthIndex, numDepthValues);
    for (Factor f : factorList) {
      float x = factorToX(f, factorList);
      f.setX(x);
      f.setY(y);
    }
  }

  println("Rendering...");
  for (Factor f : factors.values()) f.render(factors);
  println("Did render.");
}

void draw() {
}
