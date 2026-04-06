package jig;

import com.android.tools.smali.dexlib2.AccessFlags;
import com.android.tools.smali.dexlib2.DexFileFactory;
import com.android.tools.smali.dexlib2.Opcode;
import com.android.tools.smali.dexlib2.Opcodes;
import com.android.tools.smali.dexlib2.dexbacked.DexBackedDexFile;
import com.android.tools.smali.dexlib2.iface.ClassDef;
import com.android.tools.smali.dexlib2.iface.Field;
import com.android.tools.smali.dexlib2.iface.Method;
import com.android.tools.smali.dexlib2.iface.MethodImplementation;
import com.android.tools.smali.dexlib2.iface.instruction.Instruction;
import com.android.tools.smali.dexlib2.iface.instruction.ReferenceInstruction;
import com.android.tools.smali.dexlib2.iface.reference.StringReference;
import com.android.tools.smali.dexlib2.immutable.ImmutableClassDef;
import com.android.tools.smali.dexlib2.immutable.ImmutableMethod;
import com.android.tools.smali.dexlib2.immutable.ImmutableMethodImplementation;
import com.android.tools.smali.dexlib2.immutable.instruction.ImmutableInstruction10x;
import com.android.tools.smali.dexlib2.immutable.instruction.ImmutableInstruction21c;
import com.android.tools.smali.dexlib2.immutable.instruction.ImmutableInstruction35c;
import com.android.tools.smali.dexlib2.immutable.reference.ImmutableMethodReference;
import com.android.tools.smali.dexlib2.immutable.reference.ImmutableStringReference;
import com.android.tools.smali.dexlib2.writer.io.FileDataStore;
import com.android.tools.smali.dexlib2.writer.pool.DexPool;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class DexPatcher {

    public static void main(String[] args) {
        if (args.length != 3) {
            System.err.println("Usage: jig-dex-patcher <input.dex> <class.name> <output.dex>");
            System.exit(1);
        }

        String inputPath = args[0];
        String className = args[1];
        String outputPath = args[2];

        // Convert dotted class name to Dalvik descriptor: com.example.Foo -> Lcom/example/Foo;
        String descriptor = "L" + className.replace('.', '/') + ";";

        try {
            DexBackedDexFile dexFile = DexFileFactory.loadDexFile(new File(inputPath), Opcodes.getDefault());

            boolean classFound = false;
            List<ClassDef> classes = new ArrayList<>();

            for (ClassDef classDef : dexFile.getClasses()) {
                if (classDef.getType().equals(descriptor)) {
                    classFound = true;

                    if (isAlreadyPatched(classDef)) {
                        System.out.println("Already patched: " + className);
                        classes.add(classDef);
                    } else {
                        classes.add(patchClass(classDef, descriptor));
                        System.out.println("Patched: " + className);
                    }
                } else {
                    classes.add(classDef);
                }
            }

            if (!classFound) {
                System.err.println("Class not found: " + className);
                System.exit(2);
            }

            // Write the modified DEX
            DexPool dexPool = new DexPool(Opcodes.getDefault());
            for (ClassDef c : classes) {
                dexPool.internClass(c);
            }
            dexPool.writeTo(new FileDataStore(new File(outputPath)));

        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }

    /**
     * Check if the class already has System.loadLibrary("jig") injected.
     */
    private static boolean isAlreadyPatched(ClassDef classDef) {
        for (Method method : classDef.getDirectMethods()) {
            if (method.getName().equals("<clinit>")) {
                MethodImplementation impl = method.getImplementation();
                if (impl == null) return false;
                for (Instruction instruction : impl.getInstructions()) {
                    if (instruction.getOpcode() == Opcode.CONST_STRING) {
                        ReferenceInstruction refInst = (ReferenceInstruction) instruction;
                        if (refInst.getReference() instanceof StringReference) {
                            StringReference strRef = (StringReference) refInst.getReference();
                            if ("jig".equals(strRef.getString())) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    /**
     * Patch the class by prepending System.loadLibrary("jig") to its <clinit>.
     */
    private static ClassDef patchClass(ClassDef classDef, String descriptor) {
        List<Method> directMethods = new ArrayList<>();
        List<Method> virtualMethods = new ArrayList<>();
        boolean hasClinit = false;

        // Separate direct and virtual methods
        for (Method method : classDef.getDirectMethods()) {
            if (method.getName().equals("<clinit>")) {
                hasClinit = true;
                directMethods.add(patchClinit(method));
            } else {
                directMethods.add(method);
            }
        }
        for (Method method : classDef.getVirtualMethods()) {
            virtualMethods.add(method);
        }

        // If no <clinit> exists, create one
        if (!hasClinit) {
            directMethods.add(createClinit());
        }

        // Split fields into static and instance
        List<Field> staticFields = new ArrayList<>();
        List<Field> instanceFields = new ArrayList<>();
        for (Field field : classDef.getFields()) {
            if (AccessFlags.STATIC.isSet(field.getAccessFlags())) {
                staticFields.add(field);
            } else {
                instanceFields.add(field);
            }
        }

        return new ImmutableClassDef(
                classDef.getType(),
                classDef.getAccessFlags(),
                classDef.getSuperclass(),
                classDef.getInterfaces(),
                classDef.getSourceFile(),
                classDef.getAnnotations(),
                staticFields,
                instanceFields,
                directMethods,
                virtualMethods
        );
    }

    /**
     * Prepend System.loadLibrary("jig") instructions to an existing <clinit>.
     */
    private static Method patchClinit(Method method) {
        MethodImplementation oldImpl = method.getImplementation();
        List<Instruction> newInstructions = new ArrayList<>();

        // Prepend: const-string v0, "jig"
        newInstructions.add(new ImmutableInstruction21c(
                Opcode.CONST_STRING,
                0, // v0
                new ImmutableStringReference("jig")
        ));

        // Prepend: invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
        newInstructions.add(new ImmutableInstruction35c(
                Opcode.INVOKE_STATIC,
                1, // argument count
                0, // v0
                0, 0, 0, 0, // unused registers
                new ImmutableMethodReference(
                        "Ljava/lang/System;",
                        "loadLibrary",
                        Collections.singletonList("Ljava/lang/String;"),
                        "V"
                )
        ));

        // Append all existing instructions
        if (oldImpl != null) {
            for (Instruction instruction : oldImpl.getInstructions()) {
                newInstructions.add(instruction);
            }
        }

        // Register count: at least 1 (for v0), but keep the old count if higher
        int registerCount = 1;
        if (oldImpl != null && oldImpl.getRegisterCount() > registerCount) {
            registerCount = oldImpl.getRegisterCount();
        }

        ImmutableMethodImplementation newImpl = new ImmutableMethodImplementation(
                registerCount,
                newInstructions,
                oldImpl != null ? new ArrayList<>(oldImpl.getTryBlocks()) : null,
                null // debug items
        );

        return new ImmutableMethod(
                method.getDefiningClass(),
                method.getName(),
                method.getParameters(),
                method.getReturnType(),
                method.getAccessFlags(),
                method.getAnnotations(),
                method.getHiddenApiRestrictions(),
                newImpl
        );
    }

    /**
     * Create a new <clinit> with System.loadLibrary("jig") + return-void.
     */
    private static Method createClinit() {
        List<Instruction> instructions = new ArrayList<>();

        // const-string v0, "jig"
        instructions.add(new ImmutableInstruction21c(
                Opcode.CONST_STRING,
                0,
                new ImmutableStringReference("jig")
        ));

        // invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
        instructions.add(new ImmutableInstruction35c(
                Opcode.INVOKE_STATIC,
                1,
                0,
                0, 0, 0, 0,
                new ImmutableMethodReference(
                        "Ljava/lang/System;",
                        "loadLibrary",
                        Collections.singletonList("Ljava/lang/String;"),
                        "V"
                )
        ));

        // return-void
        instructions.add(new ImmutableInstruction10x(Opcode.RETURN_VOID));

        ImmutableMethodImplementation impl = new ImmutableMethodImplementation(
                1, // register count (v0)
                instructions,
                null, // try blocks
                null  // debug items
        );

        return new ImmutableMethod(
                "", // defining class — ImmutableClassDef fills this
                "<clinit>",
                Collections.emptyList(),
                "V",
                AccessFlags.STATIC.getValue() | AccessFlags.CONSTRUCTOR.getValue(),
                null, // annotations
                null, // hidden API restrictions
                impl
        );
    }
}
