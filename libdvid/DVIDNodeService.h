/*!
 * This file defines an API for accessing the DVID version node REST
 * interface.  Only a subset of the REST interface is implemented.
 *
 * Note: to be thread safe instantiate a unique node service
 * object for each thread.
 *
 * TODO: expand API and load node meta on initialization.
 *
 * \author Stephen Plaza (plazas@janelia.hhmi.org)
*/

#ifndef DVIDNODESERVICE_H
#define DVIDNODESERVICE_H

#include "BinaryData.h"
#include "DVIDVoxels.h"
#include "DVIDGraph.h"
#include "DVIDConnection.h"
#include "DVIDBlocks.h"
#include "DVIDRoi.h"

#include <json/value.h>
#include <vector>
#include <fstream>
#include <string>

namespace libdvid {

//! Creates type for DVID unique identifier string
typedef std::string UUID;

//! Used to define the relevant orthogonal cut-plane
enum Slice2D { XY, XZ, YZ };


/*!
 * Class that helps access different DVID version node actions.
*/
class DVIDNodeService {
  public:
    /*!
     * Constructor sets up a http connection and checks
     * whether a node of the given uuid and web server exists.
     * \param web_addr_ address of DVID server
     * \param uuid_ uuid corresponding to a DVID node
    */
    DVIDNodeService(std::string web_addr_, UUID uuid_);

    /*!
     * Allow client to specify a custom http request with an
     * http endpoint for a given node and uuid.  A request
     * to /node/<uuid>/blah should provide the endpoint
     * as '/blah'.
     * \param endpoint REST endpoint given the node's uuid
     * \param payload binary data to be sent in the request
     * \param method http verb (GET, PUT, POST, DELETE)
     * \return http response as binary data
    */
    BinaryDataPtr custom_request(std::string endpoint, BinaryDataPtr payload,
            ConnectionMethod method);

    /*!
     * Retrieves meta data for a given datatype instance
     * \param datatype_name name of datatype instance
     * \return JSON describing instance meta data
    */
    Json::Value get_typeinfo(std::string datatype_name);

    /************* API to create datatype instances **************/
    // TODO: pass configuration data.
    // WARNING: DO NOT USE '-' IN NAMES FOR NOW
    
    /*!
     * Create an instance of uint8 grayscale datatype.
     * \param datatype_name name of new datatype instance
     * \return true if create, false if already exists
    */
    bool create_grayscale8(std::string datatype_name);
    
    /*!
     * Create an instance of uint64 labelblk datatype and optionally
     * create a label volume datatype.  WARNING: If the function returns false
     * and a label volume is requested it is possible that the two
     * datatypes created will not be synced together.  Currently,
     * the syncing configuration needs to be set on creation.
     * \param datatype_name name of new datatype instance
     * \param labelvol_name name of labelvolume to associate with labelblks
     * \return true if both created, false if one already exists
    */
    bool create_labelblk(std::string datatype_name,
            std::string labelvol_name = "");
    
    /*!
     * Create an instance of keyvalue datatype.
     * \param datatype_name name of new datatype instance
     * \return true if create, false if already exists
    */
    bool create_keyvalue(std::string keyvalue);
    
    /*!
     * Create an instance of labelgraph datatype.
     * \param name name of new datatype instance
     * \return true if create, false if already exists
    */
    bool create_graph(std::string name);

    /*!
     * Create an instance of ROI datatype.
     * \param name name of new datatype instance
     * \return true if create, false if already exists
    */
    bool create_roi(std::string name);

    /********** API to access labels and grayscale data **********/   
    // TODO: maybe support custom byte buffers for getting and putting 

    /*!
     * Retrive a pre-computed tile from DVID at the specified
     * location and zoom level.
     * \param datatype_instance name of tile type instance
     * \param slice specify XY, YZ, or XZ
     * \param scaling specify zoom level (1=max res)
     * \param tile_loc X,Y,Z location of tile (X and Y are in tile coordinates)
     * \return 2D grayscale object that wraps a byte buffer
    */ 
    Grayscale2D get_tile_slice(std::string datatype_instance, Slice2D slice,
            unsigned int scaling, std::vector<int> tile_loc);

    /*!
     * Retrive the raw pre-computed tile (no decompression) from
     * DVID at the specified location and zoom level.  In theory, this
     * could be applied to multi-scale label data, but DVID typically
     * only stores tiles for grayscale data since it is immutable.
     * \param datatype_instance name of tile type instance
     * \param slice specify XY, YZ, or XZ
     * \param scaling specify zoom level (1=max res)
     * \param tile_loc e.g., X,Y,Z location of tile (X and Y are in block coordinates
     * \return byte buffer for the raw compressed data stored (e.g, JPEG or PNG) 
    */ 
    BinaryDataPtr get_tile_slice_binary(std::string datatype_instance, Slice2D slice,
            unsigned int scaling, std::vector<int> tile_loc);

    /*!
     * Retrive a 3D 1-byte grayscale volume with the specified
     * dimension size and spatial offset.  The dimension
     * sizes and offset default to X,Y,Z (the
     * DVID 0,1,2 channel).  The data is returned so X corresponds
     * to the matrix column.  Because it is easy to overload a single
     * server implementation of DVID with hundreds of volume requests,
     * we support a throttle command that prevents multiple volume
     * GETs/PUTs from executing at the same time.
     * A 2D slice should be requested as X x Y x 1.  The requested
     * number of voxels cannot be larger than INT_MAX/8.
     * \param datatype_instance name of grayscale type instance
     * \param dims size of X, Y, Z dimensions in voxel coordinates
     * \param offset X, Y, Z offset in voxel coordinates
     * \param throttle allow only one request at time (default: true)
     * \param compress enable lz4 compression
     * \param roi specify DVID roi to mask GET operation (return 0s outside ROI)
     * \return 3D grayscale object that wraps a byte buffer
    */
    Grayscale3D get_gray3D(std::string datatype_instance, Dims_t dims,
            std::vector<int> offset, bool throttle=true,
            bool compress=false, std::string roi="");

    /*!
     * Retrive a 3D 1-byte grayscale volume with the specified
     * dimension size and spatial offset.  However, the user
     * can also specify the channel order
     * of the retrieved volume.  The default is X, Y, Z (or 0, 1, 2).
     * Specfing (1,0,2) will allow the returned data where the column
     * dimension corresponds to Y instead of X. Because it is easy to
     * overload a single server implementation of DVID with hundreds
     * of volume requests, we support a throttle command that prevents
     * multiple volume GETs/PUTs from executing at the same time.
     * A 2D slice should be requested as ch1 size x ch2 size x 1.  The requested
     * number of voxels cannot be larger than INT_MAX/8.
     * \param datatype_instance name of grayscale type instance
     * \param dims size of dimensions (order given by channels)
     * \param offset offset in voxel coordinates (order given by channels)
     * \param channels channel order (default: 0,1,2)
     * \param throttle allow only one request at time (default: true)
     * \param compress enable lz4 compression
     * \param roi specify DVID roi to mask GET operation (return 0s outside ROI)
     * \return 3D grayscale object that wraps a byte buffer
    */
    Grayscale3D get_gray3D(std::string datatype_instance, Dims_t dims,
            std::vector<int> offset,
            std::vector<unsigned int> channels, bool throttle=true,
            bool compress=false, std::string roi="");
    
     /*!
     * Retrive a 3D 8-byte label volume with the specified
     * dimension size and spatial offset.  The dimension
     * sizes and offset default to X,Y,Z (the
     * DVID 0,1,2 channel).  The data is returned so X corresponds
     * to the matrix column.  Because it is easy to overload a single
     * server implementation of DVID with hundreds of volume requests,
     * we support a throttle command that prevents multiple volume
     * GETs/PUTs from executing at the same time.
     * A 2D slice should be requested as X x Y x 1.  The requested
     * number of voxels cannot be larger than INT_MAX/8.
     * \param datatype_instance name of the labelblk type instance
     * \param dims size of X, Y, Z dimensions in voxel coordinates
     * \param offset X, Y, Z offset in voxel coordinates
     * \param throttle allow only one request at time (default: true)
     * \param compress enable lz4 compression
     * \param roi specify DVID roi to mask GET operation (return 0s outside ROI)
     * \return 3D label object that wraps a byte buffer
    */
    Labels3D get_labels3D(std::string datatype_instance, Dims_t dims,
            std::vector<int> offset, bool throttle=true,
            bool compress=true, std::string roi="");
   
    /*!
     * Retrive a 3D 8-byte label volume with the specified
     * dimension size and spatial offset.  However, the user
     * can also specify the channel order
     * of the retrieved volume.  The default is X, Y, Z (or 0, 1, 2).
     * Specfing (1,0,2) will allow the returned data where the column
     * dimension corresponds to Y instead of X. Because it is easy to
     * overload a single server implementation of DVID with hundreds
     * of volume requests, we support a throttle command that prevents
     * multiple volume GETs/PUTs from executing at the same time.
     * A 2D slice should be requested as ch1 size x ch2 size x 1.  The requested
     * number of voxels cannot be larger than INT_MAX/8.
     * \param datatype_instance name of the labelblk type instance
     * \param dims size of dimensions (order given by channels)
     * \param offset offset in voxel coordinates (order given by channels)
     * \param channels channel order (default: 0,1,2)
     * \param throttle allow only one request at time (default: true)
     * \param compress enable lz4 compression
     * \param roi specify DVID roi to mask GET operation (return 0s outside ROI)
     * \return 3D label object that wraps a byte buffer
    */
    Labels3D get_labels3D(std::string datatype_instance, Dims_t dims,
            std::vector<int> offset,
            std::vector<unsigned int> channels, bool throttle=true,
            bool compress=true, std::string roi="");

    /*
     * Retrieve label id at the specified point.  If no ID is found, return 0.
     * \param datatype_instance name of the labelblk type instance
     * \param x x location
     * \param y y location
     * \param z z location
     * \return body id for given location (0 if none found)
    */
    uint64 get_label_by_location(std::string datatype_instance, unsigned int x,
            unsigned int y, unsigned int z);

    /*!
     * Put a 3D 1-byte grayscale volume to DVID with the specified
     * dimension and spatial offset.  THE DIMENSION AND OFFSET ARE
     * IN VOXEL COORDINATS BUT MUST BE BLOCK ALIGNED.  The size
     * of DVID blocks are determined at repo creation and is
     * always 32x32x32 currently.  The channel order is always
     * X, Y, Z.  Because it is easy to overload a single server
     * implementation of DVID with hundreds
     * of volume PUTs, we support a throttle command that prevents
     * multiple volume GETs/PUTs from executing at the same time.
     * The number of voxels put cannot be larger than INT_MAX/8.
     * TODO: expose block size parameter through interface.
     * \param datatype_instance name of the grayscale type instance
     * \param volume grayscale 3D volume encodes dimension sizes and binary buffer 
     * \param offset offset in voxel coordinates (order given by channels)
     * \param throttle allow only one request at time (default: true)
     * \param compress enable lz4 compression
    */
    void put_gray3D(std::string datatype_instance, Grayscale3D const & volume,
            std::vector<int> offset, bool throttle=true,
            bool compress=false);

    /*!
     * Put a 3D 8-byte label volume to DVID with the specified
     * dimension and spatial offset.  THE DIMENSION AND OFFSET ARE
     * IN VOXEL COORDINATS BUT MUST BE BLOCK ALIGNED.  The size
     * of DVID blocks are determined at repo creation and is
     * always 32x32x32 currently.  The channel order is always
     * X, Y, Z.  Because it is easy to overload a single server
     * implementation of DVID with hundreds
     * of volume PUTs, we support a throttle command that prevents
     * multiple volume GETs/PUTs from executing at the same time.
     * The number of voxels put cannot be larger than INT_MAX/8.
     * TODO: expose block size parameter through interface.
     * \param datatype_instance name of the grayscale type instance
     * \param volume label 3D volume encodes dimension sizes and binary buffer 
     * \param offset offset in voxel coordinates (order given by channels)
     * \param throttle allow only one request at time (default: true)
     * \param roi specify DVID roi to mask PUT operation (default: empty)
     * \param compress enable lz4 compression
    */
    void put_labels3D(std::string datatype_instance, Labels3D const & volume,
            std::vector<int> offset, bool throttle=true,
            bool compress=true, std::string roi="");

    /************** API to access DVID blocks directly **************/
    // This API is probably most relevant for bulk transfers to and
    // from DVID where high-throughput needs to be optimized.
    
    /*!
     * Fetch grayscale blocks from DVID.  The call will fetch
     * a series of contiguous blocks along the first dimension (X).
     * The number of blocks fetched is encoded in the GrayscaleBlocks
     * returned structure. 
     * TODO: support compression and throttling.
     * \param datatype instance name of grayscale type instance
     * \param block_coords location of first block in span (block coordinates) (X,Y,Z)
     * \param span number of blocks to attemp to read
     * \return grayscale blocks
    */
    GrayscaleBlocks get_grayblocks(std::string datatype_instance,
           std::vector<int> block_coords, unsigned int span); 

    /*!
     * Fetch label blocks from DVID.  The call will fetch
     * a series of contiguous blocks along the first dimension (X).
     * The number of blocks fetched is encoded in the LabelBlocks
     * returned structure.
     * TODO: support compression and throttling.
     * \param datatype instance name of labelblk type instance
     * \param block_coords location of first block in span (block coordinates) (X,Y,Z)
     * \param span number of blocks to attemp to read
     * \return grayscale blocks
    */
    LabelBlocks get_labelblocks(std::string datatype_instance,
           std::vector<int> block_coords, unsigned int span);

    /*!
     * Put grayscale blocks to DVID.   The call will put
     * a series of contiguous blocks along the first spatial dimension (X).
     * The number of blocks posted is encoded in GrayscaleBlocks.
     * TODO: support compression and throttling.
     * \param datatype instance name of grayscale type instance
     * \param blocks stores buffer for array of blocks
     * \param block_coords location of first block in span (block coordinates) (X,Y,Z)
    */
    void put_grayblocks(std::string datatype_instance,
            GrayscaleBlocks blocks, std::vector<int> block_coords);
    
    /*!
     * Put label blocks to DVID.   The call will put
     * a series of contiguous blocks along the first spatial dimension (X).
     * The number of blocks posted is encoded in LabelBlocks.
     * TODO: support compression and throttling.
     * NOTE: UNTESTED (DVID DOES NOT YET SUPPORT)
     * \param datatype instance name of labelblk type instance
     * \param blocks stores buffer for array of blocks
     * \param block_coords location of first block in span (block coordinates) (X,Y,Z)
    */
    void put_labelblocks(std::string datatype_instance,
            LabelBlocks blocks, std::vector<int> block_coords);

    /*************** API to access keyvalue interface ***************/
    
    /*!
     * Put binary blob at a given key location.  It will overwrite data
     * that exists at the key for the given node version.
     * \param keyvalue name of keyvalue instance
     * \param key name of key to the keyvalue instance
     * \param value binary blob to store at key
    */
    void put(std::string keyvalue, std::string key, BinaryDataPtr value);
    
    /*!
     * Put data in a file at a given key location.  It will overwrite
     * data that exists at the key for the given node version.
     * \param keyvalue name of keyvalue instance
     * \param key name of key to the keyvalue instance
     * \param fin file stream that contains binary to store
    */
    void put(std::string keyvalue, std::string key, std::ifstream& fin);

    /*!
     * Put JSON data at a given key location.  It will overwrite data
     * that exists at the key for the given node version.
     * \param keyvalue name of keyvalue instance
     * \param key name of key to the keyvalue instance
     * \param data JSON data to store at key
    */
    void put(std::string keyvalue, std::string key, Json::Value& data);

    /*!
     * Retrive binary data at a given key location.
     * \param keyvalue name of keyvalue instance
     * \param key name of key to the keyvalue instance
     * \return binary data stored at key
    */
    BinaryDataPtr get(std::string keyvalue, std::string key);
    // could return a reference but assuming that this is used for short messages
    
    /*!
     * Retrieve json of data at a given key location.
     * \param keyvalue name of keyvalue instance
     * \param key name of key to the keyvalue instance
     * \return json stored at key
    */
    Json::Value get_json(std::string keyvalue, std::string key);
    
    /************** API to access labelgraph interface **************/
   
    /*!
     * Download the graph into the graph datatype.  If vertices are supplied,
     * a subgraph is extracted that includes just those vertices.  This
     * command could be time-consuming for large graphs.
     * \param graph_name name of labelgraph instance
     * \param vertices if no vertices are specified, retrieve the whole graph
     * \param graph the resulting graph (vertice, edges, and edge weights)
    */
    void get_subgraph(std::string graph_name,
            const std::vector<Vertex>& vertices, Graph& graph);

    /*!
     * Extract all the vertices connected to a paritcular vertex.
     * This is a low latency call.
     * \param graph_name name of labelgraph instance
     * \param vertex grab vertices connected to this vertex
     * \param graph vertex and partners are stored in the graph type
    */
    void get_vertex_neighbors(std::string graph_name, Vertex vertex,
            Graph& graph);

    /*!
     * Add the provided vertices to the labelgraph with the associated
     * vertex weights.  If the vertex already exists, it will increment
     * the vertex weight by the weight specified.  This function
     * can be used for creation and incrementing vertex weights in parallel.
     * \param graph_name name of labelgraph instance
     * \param vertices list of vertices to create or update
    */ 
    void update_vertices(std::string graph_name,
            const std::vector<Vertex>& vertices);
    
    /*!
     * Add the provided edges to the labelgraph with the associated
     * edge weights.  If the edge already exists, it will increment
     * the vertex weight by the weight specified.  This function
     * can be used for creation and incrementing edge weights in parallel.
     * The command will fail if the vertices for the given edges
     * were not created first.
     * \param graph_name name of labelgraph instance
     * \param vertices list of vertices to create or update
    */ 
    void update_edges(std::string graph_name,
            const std::vector<Edge>& edges);

    /*!
     * Retrieve properties associated with a list of vertices.  Binary
     * is returned as array that corresponds to the list of vertices. This
     * command can be used to get a transaction ID for each vertex.
     * These transaction IDs must be used when one wants to update
     * a property.  It ensures that the property was not modified
     * by another client.
     * \param graph_name name of labelgraph instance
     * \param vertices properties are retrieved for these vertices
     * \param key name of property
     * \param properties properties corresponding to the vertex list
     * \param transactions returns transaction ids for all vertices
    */
    void get_properties(std::string graph_name,
            std::vector<Vertex> vertices, std::string key,
            std::vector<BinaryDataPtr>& properties,
            VertexTransactions& transactions);

    /*!
     * Retrieve properties associated with a list of edges.  Binary
     * is returned as array that corresponds to the list of edges. This
     * command can be used to get a transaction ID for each vertex
     * that corresponds to the list of edges.
     * These transaction IDs must be used when one wants to update
     * a property.  It ensures that the property was not modified
     * by another client.
     * \param graph_name name of labelgraph instance
     * \param edges properties are retrieved for these edges
     * \param key name of property
     * \param properties properties corresponding to the edge list
     * \param transactions returns transaction ids for all edge vertices
    */
    void get_properties(std::string graph_name, std::vector<Edge> edges,
            std::string key, std::vector<BinaryDataPtr>& properties,
            VertexTransactions& transactions);

    /*!
     * Set properties as binary blobs for a list of vertices.
     * Must provide transaction ids for each vertex being written
     * to.  These IDs are retrieved use the get_properties command.
     * Any vertex with a stale transaction id is returned.
     * \param graph_name name of labelgraph instance
     * \param vertices properties are set for these vertices
     * \param key name of property
     * \param properties binary blobs to be set
     * \param transaction specify transactions for set call
     * \param leftover_vertices vertices that could not be written
    */
    void set_properties(std::string graph_name,
            std::vector<Vertex>& vertices, std::string key,
            std::vector<BinaryDataPtr>& properties,
            VertexTransactions& transactions,
            std::vector<Vertex>& leftover_vertices);

    /*!
     * Set properties as binary blobs for a list of edges.
     * Must provide transaction ids for vertices of each edge
     * being written to.  These IDs are retrieved use the
     * get_properties command. Any vertex with a stale transaction
     * id is returned.
     * \param graph_name name of labelgraph instance
     * \param edges properties are set for these edges
     * \param key name of property
     * \param properties binary blobs to be set
     * \param transaction specify transactions for set call
     * \param leftover_vertices vertices that could not be written
    */
    void set_properties(std::string graph_name,
            std::vector<Edge>& edges, std::string key,
            std::vector<BinaryDataPtr>& properties,
            VertexTransactions& transactions,
            std::vector<Edge>& leftover_edges);
    
    /************** API to access ROI interface **************/
    // Currently, there is no API to work directly on the RLE
    // encoded blocks.  This might lead to excessive memory use
    // and runtime for some use cases.  Furthermore, this API
    // handles block and substack ordering (regardless of whether
    // it is necessary or whether it is already sorted).  This
    // might lead to some runtime inefficiencies.

    /*!
     * Load an ROI defined by a list of blocks.  This command
     * will extend the ROI if it defines blocks outside of the
     * currently defined ROI.  The blocks can be provided in
     * any order.
     * \param roi_name name of the roi instance
     * \param blockcoords vector of block coordinates
    */
    void post_roi(std::string roi_name,
            const std::vector<BlockXYZ>& blockcoords);
   
    /*!
     * Retrieve an ROI and store in a vector of block coordinates.
     * The blocks returned will be ordered by Z then Y then X.
     * \param roi_name name of the roi instance
     * \param blockcoords vector of block coordinates retrieved
    */
    void get_roi(std::string roi_name,
            std::vector<BlockXYZ>& blockcoords);
    
    /*!
     * Retrieve a partition of the ROI covered by substacks
     * of the specified partition size.  The substacks will be ordered
     * by Z then Y then X.
     * \param roi_name name of the roi instance
     * \param substacks vector of substacks that coer the ROI
     * \param partition_size substack size as number of blocks in one dimension
     * \return fraction of substack volume that cover blocks (packing factor)
    */
    double get_roi_partition(std::string roi_name,
            std::vector<SubstackXYZ>& substacks, unsigned int partition_size);

    /*!
     * Check whether a list of points (any order) exists in
     * the given ROI.  A vector of true and false has the same order
     * as the list of points.
     * \param roi_name name of the roi instance
     * \param points list of X,Y,Z points
     * \param inroi list of true/false on whether points are in the ROI
    */
    void roi_ptquery(std::string roi_name,
            const std::vector<PointXYZ>& points,
            std::vector<bool>& inroi);

    /************** API to access sparse body interface **************/
    // The current functionality is working over the coarse volume
    // endpoint available in DVID.  The coarse volume is just a list of
    // blocks that intersect the body.  Some of the functions are
    // workarounds or approximations that use the coarse volume.

    /*!
     * Determine whether body exists in labelvolume.
     * \param labelvol_name name of label volume type
     * \param bodyid body id being queried
     * \return true if in label volume, false otherwise
    */
    bool body_exists(std::string labelvol_name, uint64 bodyid);

    /*!
     * Find a point in the center of the  body (currently an
     * approximate location is chosen).  If a third dimension coordinate
     * is provided, a point is provided within that 'Z' plane if it
     * exists, otherwise the center point is chosen.
     * \param labelvol_name name of label volume type
     * \param bodyid body id being queried
     * \param zplane restrict body location to this plane
     * \return point representing body location
    */  
    PointXYZ get_body_location(std::string labelvol_name, uint64 bodyid,
           int zplane=INT_MAX);

    /*!
     * Retrieve coarse volume for given body ID as a vector
     * of blocks in block coordinates.
     * \param labelvol_name name of label volume type
     * \param bodyid body id being queried
     * \param blockcoords vector of block coordinates retrieved for body
     * \return false if body does not exist
    */
    bool get_coarse_body(std::string labelvol_name, uint64 bodyid,
            std::vector<BlockXYZ>& blockcoords);

  private:
    //! HTTP connection with DVID
    DVIDConnection connection;
    
    //! uuid for instance
    const UUID uuid;

    /*!
     * Helper function to put a 3D volume to DVID with the specified
     * dimension and spatial offset.  THE DIMENSION AND OFFSET ARE
     * IN VOXEL COORDINATS BUT MUST BE BLOCK ALIGNED. 
     * \param datatype_instance name of tile type instance
     * \param volume binary buffer encodes volume 
     * \param offset offset in voxel coordinates (order given by channels)
     * \param throttle allow only one request at time
     * \param compress enable lz4 compression
     * \param roi specify DVID roi to mask PUT operation (default: empty)
    */
    void put_volume(std::string datatype_instance, BinaryDataPtr volume,
            std::vector<unsigned int> sizes, std::vector<int> offset,
            bool throttle, bool compress, std::string roi);

    /*!
     * Helper to retrieve blocks from DVID for labels and grayscale.
     * \param datatype_instance name of datatype instance
     * \param block_coords starting block in DVID block coordinates
     * \param span number of blocks to attempt to get
     * \return binary data corresponding to an array of blocks
    */
    BinaryDataPtr get_blocks(std::string datatype_instance,
        std::vector<int> block_coords, int span);

    /*!
     * Helper to put blocks from DVID for labels and grayscale.
     * \param datatype_instance name of datatype instance
     * \param binary array of blocks to be written
     * \param span number of blocks to attempt to post
     * \param block_coord starting block in DVID block coordinates
    */
    void put_blocks(std::string datatype_instance, BinaryDataPtr binary,
            int span, std::vector<int> block_coords);

    /*!
     * Helper function to create an instance of the specified type.
     * \param datatype name of the datatype to create
     * \param datatype_name name of new datatype instance
     * \param syn_name dataname to sync with if provided
     * \return true if create, false if already exists
    */
    bool create_datatype(std::string datatype, std::string datatype_name,
            std::string sync_name = "");

    /*!
     * Checks if data exists for the given datatype name.
     * \return true if the instance already exists
    */
    bool exists(std::string datatype_endpoint);

    /*!
     * Helper function to retrieve a 3D volume with the specified
     * dimension size, spatial offset, and channel retrieval order.
     * \param datatype_instance name of tile type instance
     * \param dims size of dimensions (order given by channels)
     * \param offset offset in voxel coordinates (order given by channels)
     * \param channels channel order (default: 0,1,2)
     * \param throttle allow only one request at time
     * \param compress enable lz4 compression
     * \param roi specify DVID roi to mask GET operation (return 0s outside ROI)
     * \return byte buffer corresponding to volume
    */
    BinaryDataPtr get_volume3D(std::string datatype_inst, Dims_t sizes,
        std::vector<int> offset, std::vector<unsigned int> channels,
        bool throttle, bool compress, std::string roi);

    /*!
     * Helper function to construct a REST endpoint strign for
     * volume GETs and PUTs given several parameters.
     * \param datatype_instance name of tile type instance
     * \param dims size of dimensions (order given by channels)
     * \param offset offset in voxel coordinates (order given by channels)
     * \param channels channel order (default: 0,1,2)
     * \param throttle allow only one request at time
     * \param compress enable lz4 compression
     * \param roi specify DVID roi to mask operation (default: empty)
    */
    std::string construct_volume_uri(std::string datatype_inst, Dims_t sizes,
            std::vector<int> offset,
            std::vector<unsigned int> channels, bool throttle, bool compress,
            std::string roi);
};

}

#endif
